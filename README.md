# FFSM — French Forest Sector Model


French Forest Sector Model is a bio-economic model of the forest sector - forest dynamic, forest management and timber markets.

The current version is implemented in Julia (previous versions were in GAMS and C++).

## Running

From this folder:

```bash
julia --project=. -e 'using Pkg; Pkg.instantiate()'    # first time only
julia --project=. ffsm.jl -s default                  # scenario "default" of data/ffsmInput.ods
julia --project=. ffsm.jl -i data/ffsmInput.ods -s debug
```

The options are those of the C++ binary: `-i` input file (relative to the current directory),
`-s` scenario (default: the first one in the `scenarios` sheet), `-c` (accepted and ignored, as
the console mode is the only one) and `-h`. `--hsllib <path>` gives Ipopt the path to the HSL
library, to use the `ma27`/`ma57` linear solvers set in the `linearSolver` setting.

From Julia: `using FFSM; m = runModel("data/ffsmInput.ods", "default")`. The returned object holds
the whole model state, which is handy for inspecting the data after a run. Two keyword arguments
have no C++ equivalent:

- `settings = Dict("simulationYears" => 5, "outputLevel" => 10)` overrides (world-level) settings
  after the scenario-specific ones have been applied;
- `ipoptOptions = Dict("tol" => 1e-9)` passes further options to Ipopt.

As in the C++ version, the base directory is the folder of the input file, and the outputs go
to the folder given by the `outputDirname` setting (`data/output/`).

## Layout

| File | Content | C++ equivalent |
|---|---|---|
| `src/FFSM.jl` | module definition | |
| `src/utils.jl` | messages, conversions and C-compatible helpers | `BaseClass` |
| `src/rng.jl` | random generator: a `StableRNG` seeded with the `randomSeed` setting (or random with `newRandomSeed`) | `<random>` |
| `src/odsreader.jl` | ODS (content.xml) reader | `ModelData::loadInput`, `LLData`, `InputNode` |
| `src/datamap.jl` | the forest/product data maps, including prefix queries | `std::map<string,vector<double>>` |
| `src/modeldata.jl` | settings, regions, data access and regional overriding | `ModelData`, `ModelRegion` |
| `src/gis.jl` | pixels, layers, grid loading, stochastic pixel values | `Gis`, `Layers`, `Pixel` |
| `src/carbon.jl` | carbon accounting | `Carbon` |
| `src/market.jl` | yearly market equilibrium (JuMP algebraic model, one constraint family per equation, Ipopt) | `Opt` (ADOL-C + Ipopt) |
| `src/core.jl` | init period and yearly dynamics (market, resources, management) | `ModelCoreSpatial` |
| `src/output.jl` | csv results, optimisation log, debug values, maps (ASCII grids, png) | `Output`, `Layers::print` |
| `src/run.jl` | init levels, yearly scheduler, command line | `Init`, `Scheduler`, `ThreadManager`, `main` |
| `test/runtests.jl` | test suite entry point (see [Tests](#tests)) | |
| `test/build_test_input.jl`, `test/test_layers.jl` | generators of the test input file and of the synthetic test layers (`test/data/`) | |
| `test/compare_outputs.jl` | compares the outputs of two runs (e.g. C++ vs Julia) | |

`data/` is a copy of the git-tracked content of the `data/` folder of the C++ project, without the
previous outputs. It can later be replaced by a symlink to `../data`.

## How faithful is the port

The aim is to reproduce the C++ results with the same inputs, so the port follows the C++ code
closely, including several of its peculiarities:

- **Data look-ups.** Product data asked without `freeDim` are prefix queries over the sorted keys
  (`getProdData` sums all the matching entries). A year after the last one returns the last value,
  a year before the initial year the first one, with a warning (`getTimedData`; it never happens in the
  current model). Level-1 queries sum the level-2 children. Missing data
  return 0 with the same messages.
- **Randomness.** The per-pixel risk aversion, expectation types and spatial modifiers (and the optional
  initial shuffle of the layers) are drawn with Distributions.jl from a `StableRNG` (StableRNGs.jl), whose
  streams don't change between Julia versions, seeded with the `randomSeed` setting: two runs of the same scenario give the
  same results. With `newRandomSeed = true` the seed is random (and printed in the log). Up to v0.2 the
  model used a bit-exact replica of the C++ `std::mt19937(0)` and `normal_distribution`, to reproduce the
  C++ outputs; the results are now statistically equivalent, not identical, to the C++ ones. The
  expectation types are drawn from a normal truncated to [0, 1]: the C++ `normSample` applied only the upper
  bound, as it compared the lower bound 0 with `NULL` (no bound), and gave a few slightly negative values.
- **Arithmetic.** Floating-point expressions keep the C++ evaluation order, `pow`/`exp`/`log` are
  glibc's, and `std::min`/`std::max` keep their NaN behaviour (`cmin`/`cmax`).
- **Known C++ behaviours that are reproduced:** the `pixelIds` layer stays empty, the end-of-init
  output is an "early" print (no debug pixel values for the first optimisation-free year), the
  area of the first diameter class of the destination type in `loadExogenousForestLayers` takes
  `area_l[0][0]`, the last vHa in `getVHaByYear` uses `cumTp_exp`, and `usableDeadTimber` is
  rounded to a `float`.

The market problem is the same NLP (variables, bounds, starting point, objective and constraints,
Ipopt `tol=1e-6`, `acceptable_tol=1e-5`). Derivatives are computed by JuMP instead of ADOL-C, and
Ipopt/MUMPS are the versions shipped with Ipopt.jl. Results therefore agree within the solver
tolerance, not bit-for-bit.

## Validation against the C++ binary

This validation was done up to v0.2, with the C++-compatible random numbers. Since then the model uses
`StableRNG`: the random draws, and therefore the outputs, differ from the C++ ones as with a change of seed.

Outputs were compared with `test/compare_outputs.jl` against the C++ binary (commit `7884375`) on the
same inputs:

| Scenario | Years | Result |
|---|---|---|
| `debug` (2 regions, outputLevel 25) | 2013–2112 | Objective identical every year (6 digits). All discrete pixel choices (optimal dc and forest type) identical. Pre-optimisation years identical. Other values within ~1e-5 relative (the last printed digit). 40/47 ASCII maps byte-identical, the rest differ in a few cells at the last digit; png maps identical. |
| `default` (whole France) | 2013–2112 | Objective identical every year. Everything within 2.2e-5 until 2097, up to ~3e-3 in 2098 on the Corsican market (see below), within 6e-5 afterwards. Regional trade flows are degenerate (several equivalent optimal flows). Re-validated after the rewrite of the market module as an algebraic model. |
| `Windstorm_Ref` (2025 windstorm project) | 2013–2050 | Everything within 1e-5 (except some degenerate regional trade flows). |
| `Sto_M` (2025 windstorm project) | 2013–2050 | Everything within 1e-5 except in 2045 (see below). |

The larger differences come from the very small Corsican pulpwood market (~0.0025 Mm³) in
single years (2098 in `default`, 2045 in `Sto_M`); the two versions re-converge the following year.
In these flat directions of the objective, `tol = 1e-6` gives only about 1e-4–5e-3 relative accuracy,
in **both** versions. Re-solving with `tol = 1e-11` shows that neither default-tolerance solution
is exact there, so these are not porting errors.

| Run time (single run) | C++ | Julia |
|---|---|---|
| `debug` (100 years, heavy pixel-level debug output) | 6m56s | 2m47s |
| `default` (100 years) | 32m40s | 4m28s |
| `Windstorm_Ref` / `Sto_M` (38 years, 3121 additional mortality maps) | 18m40s, 8.1 GB RAM | 2m56s–3m17s, 3.0 GB RAM |

The first run in a fresh Julia session also includes about 30–60 s of compilation.

Note: like the C++ code, the `detailedHV_*` file is appended whenever `outDetailedHv` is true, even
with `outputLevel` below 15 (in that case the file is not re-initialised).

## Tests

```bash
julia --project=. -e 'using Pkg; Pkg.test()'                       # all (~6 min)
FFSM_SKIP_SCENARIOS=1 julia --project=. -e 'using Pkg; Pkg.test()' # only the unit tests and the short runs
FFSM_TESTS=t_base,t_pathogen julia --project=. test/behaviour_tests.jl
```

- `runtests.jl`: unit tests of the helpers, short runs of the `debug` scenario, random seeding
  (`randomSeed`, `newRandomSeed`) and `initialRandomShuffle`;
- `behaviour_tests.jl`: the 46 `t_*` scenarios of `test/data/ffsm_tests.ods` (2 regions, 2013–2020, one
  change each: settings, forData/prodData/forToProd/pathRules overrides, GIS layers, output options). It
  checks the accounting identities and that each change moves the model in the expected direction with a
  reasonable magnitude.

The tests that compared the outputs with the C++ binary (`exact_tests.jl` and the reference outputs) were
removed after v0.2, when the model stopped using the C++-compatible random numbers.

After a change of the scenarios (`test/build_test_input.jl`) or of `data/ffsmInput.ods`:

```bash
julia --project=. test/build_test_input.jl            # needs zip and LibreOffice (soffice)
```

See [doc/port_to_julia/tests.md](doc/port_to_julia/tests.md) for the scenarios, what is checked and the
issues of the model found while writing the tests.


