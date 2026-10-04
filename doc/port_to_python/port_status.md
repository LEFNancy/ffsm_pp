# Python port (FFSMp): status

FFSMp (`ffsmp/`) is a port of the FFSM++ console model to Python. The Julia port (`ffsmj/`,
[../port_to_julia/port_status.md](../port_to_julia/port_status.md)) is the main port: the Python one was ported
from it, function by function, and keeps its structure, names, C++ quirks and fixes. Installation and usage:
[ffsmp/README.md](../../ffsmp/README.md). Feasibility and choices: [feasibility.md](feasibility.md).

## 1. Summary

- Same inputs (ODS spreadsheet, GIS XML file, ASCII grids) and same output files as the C++ binary in console
  mode. No GUI, as for the Julia port.
- All the 44 reproducible test scenarios give the C++ outputs within the solver tolerance; over 100 years of the
  `default` scenario the objective is identical every year and all the outputs agree within 2.2e-5 (section 5).
- Speed: `default` (100 years) in 25 minutes (C++ 35, Julia 5).
- Dependencies: Pyomo, and an `ipopt` executable (system package, conda-forge or IDAES extensions).

## 2. Mapping of the sources

| Module | Content | Julia file | C++ classes |
|---|---|---|---|
| `utils.py` | messages, conversions, C-compatible helpers | `utils.jl` | `BaseClass` |
| `rng.py` | bit-exact replica of `std::mt19937` and libstdc++ `normal_distribution` | `rng.jl` | `<random>` |
| `odsreader.py` | ODS (content.xml) reader | `odsreader.jl` | `ModelData::loadInput`, `LLData`, `InputNode` |
| `datamap.py` | forest/product data maps, prefix queries | `datamap.jl` | `std::map<string,vector<double>>` |
| `modeldata.py` | settings, regions, data access, regional overriding | `modeldata.jl` | `ModelData`, `ModelRegion` |
| `gis.py` | pixels, layers, grids, stochastic pixel values | `gis.jl` | `Gis`, `Layers`, `Pixel` |
| `carbon.py` | carbon accounting | `carbon.jl` | `Carbon` |
| `market.py` | yearly market equilibrium (Pyomo model, Ipopt) | `market.jl` (JuMP) | `Opt` (ADOL-C) |
| `core.py` | init period and yearly dynamics | `core.jl` | `ModelCoreSpatial` |
| `output.py` | csv results, optimisation log, debug values, maps | `output.jl` | `Output`, `Layers::print` |
| `run.py` | init levels, scheduler, command line | `run.jl` | `Init`, `Scheduler`, `ThreadManager`, `main` |

All the indexes are 0-based, as in the C++ code (the Julia port is 1-based).

## 3. What needed care in Python

- **C floating point semantics.** `math.pow`, `math.exp` and `math.log` raise exceptions where C returns
  `inf`/`nan`, and `x / 0.0` raises `ZeroDivisionError`. The helpers `cpow`, `cexp`, `clog` and `cdiv`
  (`utils.py`) restore the C results; `cdiv` is used wherever a denominator comes from data or model state and
  the code does not already guard it. CPython's `math` module calls the system libm (glibc), the one used by the
  C++ binary, so no special binding is needed (Julia needed one).
- **Sums.** Since Python 3.12 the built-in `sum()` of floats is compensated (more accurate, but not equal to the
  C++ left-to-right loop): the model uses explicit loops (`vSum`).
- **Random numbers.** `std::mt19937`, `generate_canonical` and `normal_distribution` are replicated bit by bit,
  so the per-pixel random values are those of C++ and Julia.
- **Memory.** Layer values are stored as `array('d')` (8 bytes per value, as in Julia), not as lists of Python
  floats (about 4 times more): the windstorm projects have about 3,000 layers.
- **Speed.** The pixel loops are written as in Julia, with some exact restructurings (no change in the
  arithmetic or in the order of the operations): per-region precomputed series and layer keys in
  `computeCumulativeData`, a per-year cache of the regional multipliers given in the forData table, the yearly
  carbon payments of the Hartman module computed once per pixel and forest type.
- **BLAS threads.** The package sets `OPENBLAS_NUM_THREADS=1` (unless already set), inherited by the `ipopt`
  executable: a multi-threaded BLAS does not help the small market problems.

## 4. The market model (`market.py`)

The market is a Pyomo model written over sets of regions and products:

- sets `R` (level-2 regions), `ROUTES` (pairs of regions of the same level-1 region), `PRI`, `SEC`, `ALL`
  (products), `COMBS` (combinations of primary products);
- variables `pl`, `pc`, `sl`, `dl` (R × ALL), `sc`, `sa` (R × PRI), `dc`, `da` (R × SEC), `rt` (ROUTES × ALL),
  all with lower bound 0, starting from the values of the previous year;
- the welfare (surpluses of consumers, producers, transformers and transport agents) is maximised; the
  constraints have the names of the C++ code (`mkeq2`, `mkeq6`, ..., `resbounds`).

The yearly data are read by `marketData()` and the model is rebuilt every year (as JuMP in the Julia port).
Pyomo writes it in the AMPL `.nl` format and runs the `ipopt` executable, the AMPL library giving Ipopt the exact
derivatives. Differences from a literal translation of the C++ code (none changes the solution):

- the transformation terms between primary products with a null coefficient (`a_pr = 0`) are left out: they are
  identically zero, and the second derivative of `max(0, a_pr·sc)^e` is not defined at zero;
- the resource bound of the empty combination of primary products (a constant row in the C++ code) is not
  written, as Pyomo does not accept constant constraints;
- the welfare is maximised through the objective sense (C++ minimises its opposite with `obj_scaling_factor = -1`):
  the Ipopt log shows the objective with the opposite sign, the optimisation log (`optimisationLogs/`) shows the
  welfare as in C++;
- the variables are ordered by Pyomo, and the Ipopt iteration count is read from the Ipopt log;
- variables returned slightly below zero by Ipopt (bound relaxation, ~1e-9) are used as such, as in C++, without
  the Pyomo warning.

A first version used CasADi (problem built once with the data as parameters, faster by ~2 s per year on
`default`); it was replaced by Pyomo for readability (see [feasibility.md](feasibility.md)). With CasADi, the
second derivative of the null transformation terms was `NaN`, which stopped Ipopt at the first iteration: this is
why these terms are left out.

## 5. Validation against the C++ binary

- **Test scenarios:** the 44 test scenarios that are reproducible by construction (all but `t_newSeed` and
  `t_shuffle`) give the C++ reference outputs within the solver tolerance (`tests/test_exact.py`; same input
  file and references as the Julia port, see [../port_to_julia/tests.md](../port_to_julia/tests.md)).
- **Full run:** `default` (whole France, 12 regions, 2013–2112), compared with `ffsmj/test/compare_outputs.jl`:
  objective identical every year (6 digits); forest data and carbon balance within 9e-6 relative; market variables
  within 2.2e-5, except some degenerate regional trade flows (up to 4e-4). This is closer to C++ than the Julia
  port (up to ~1e-3 on the small Corsican pulpwood market in single years), as the `ipopt` executable used here is
  linked to the same Ipopt (3.14.19) and MUMPS (5.8.2) libraries as the C++ binary.

## 6. Performance

| Run time (single run) | C++ | Julia | Python |
|---|---|---|---|
| `default` (100 years) | 35m17s | 4m28s | 25m01s |
| a test scenario (2 regions, 8 years) | ~45 s | ~6 s | ~27 s |
| a test scenario with Hartman carbon payments | ~45 s | ~15–25 s | ~70–80 s |

The time goes into the pixel-level loops of the resource and management modules; building the Pyomo model and
running `ipopt` take about 2 s per year on `default`. The Hartman payments are summed year by year over the
whole rotation for every pixel, forest type and diameter class, which is slow in pure Python. Further speed would
need vectorising the pixel loops (NumPy) or compiling them (Numba): a restructuring, not a port.

## 7. Tests

The tests of the Julia port ([../port_to_julia/tests.md](../port_to_julia/tests.md)) ported to pytest, on the same
test input file, synthetic GIS layers and C++ reference outputs (read from `ffsmj/test/`):

- `tests/test_units.py`: bit-exact random numbers, C++ compatible helpers, C semantics of the math helpers, map
  images, spatio-temporal layer look-up, a 2-year `debug` run against C++ values, time series bounds,
  `initialRandomShuffle`;
- `tests/test_exact.py`: every output file of every test scenario equal to the C++ one within the solver
  tolerance (can be skipped with `FFSMP_SKIP_EXACT=1`);
- `tests/test_behaviour.py`: accounting identities and direction/magnitude of the effect of each scenario; the
  partial withdrawal of a single-year additional demand is an expected failure (`xfail`), as `@test_broken` in
  Julia.

Result: 90 passed, 1 expected failure, in about 22 minutes. `FFSMP_TESTS=t_a,t_b` selects scenarios.

## 8. Deviations and limits

| Case | C++ / Julia | Python |
|---|---|---|
| `initialRandomShuffle = true` | `std::random_shuffle` / Julia `shuffle!` | `random.Random(seed).shuffle`: different permutations (sensitivity option only) |
| Ipopt iteration callback (`Running (N iter)` debug messages) | printed | not printed (Ipopt runs as an external program) |

Not ported, as for the Julia port: the GUI, the non-spatial model (`ModelCore`, `usePixelData = false`) and the
developer test code (`Sandbox`).

## 9. Open points

- The test data and the C++ references are read from `ffsmj/test/`, and `ffsmp/data/` is a third copy of the
  inputs: to be shared when the ports are reorganised.
- The Hartman module is slow in Python (section 6).
