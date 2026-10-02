# Porting FFSM++ from C++ to Julia: feasibility

*Assessment dated 2026-10-02, based on the code at commit `5bc2109`.*

**Verdict:** the port is feasible. The recommended path is a command-line-only port first. The optimisation part becomes much simpler in JuMP. The hard part is reproducing exactly how the data layer looks values up, and checking the Julia results against the C++ model. The GUI is the least portable piece and is better replaced by a post-run viewer than rebuilt.

**Sources:**
- the C++ code in `src/`;
- the early Julia attempt in `dev/ffsm/` (`ffsm.jl` and `ffsm_lib.jl`);
- the FFSM wiki (<https://ffsm.beta-lorraine.fr/wiki/en/home>) and the 2016 course slides linked from it;
- an OdsIO test on `data/ffsmInput.ods`;
- the logs of the 2025 windstorm-policy runs (`00_private/2025_windstorm_policies/`).

## What has to be ported

| Component | C++ lines | Effort in Julia |
|---|---|---|
| Market NLP (`src/Opt.cpp`) | 1,874 | **Easy.** Only about 450 lines are the actual equations. The rest (index bookkeeping, ADOL-C tapes, sparsity patterns) disappears with JuMP. Expect 250–300 lines. |
| Spatial core (`src/ModelCoreSpatial.cpp`) | 2,916 | **Medium.** Plain loops over pixels × forest types × diameter classes × products, so it translates line by line. The management module alone (Faustmann/Hartman returns, risk aversion, regeneration) is 530 lines. |
| Data layer (`src/ModelData.cpp`) | 2,199 | **Hardest to get identical.** The ODS parsing goes away, but the lookup rules must be copied exactly: scenario overrides; a level-1 region returns the sum of its children; a year past the end returns the last value; keywords like `FT_ALL` and `DIAM_PROD`; and "not found" returns 0 with a warning. |
| GIS, pixels, carbon, output, regions, init, layers | about 4,000 | **Easy to medium.** The grids are 147×192 GRASS ASCII files (about 30 lines of Julia to read). Output is plain CSV. |
| Can be dropped | about 7,000 | Sandbox and debug tests, zip/unzip, anyoption, the ADOL-C test files. Also `ModelCore` (the non-spatial core, 938 lines) if it is no longer used. |

So roughly 11k lines of C++ model logic, expected to become 4–6k lines of Julia.

**Problem size.** The Ipopt logs show about 1,900 variables and 576 equality constraints, with Ipopt taking 1–8 s per year. Each simulated year takes about 25–35 s overall, so a full scenario runs in about 18 minutes.

**Speed.** 1–8 s is slow for an NLP this small. It points to the string-keyed lookups (`gpd`/`gfd` build a key string on every call) inside the objective and constraint evaluations. A Julia version that keeps data in integer-indexed arrays should be as fast or faster. If the string-keyed design is copied literally, it will be slower.

## The early Julia attempt

`dev/ffsm/ffsm.jl` is useful as a reference for the equations, not as code to build on:
- **It models the older regional version.** That's the GAMS-derived, non-spatial model. It lacks what the C++ has added since: substitution elasticities (`es_d`), `additionalDemand`, the `pol_*` policy terms, carbon price terms in supply and objective, the `resbounds` constraints over product combinations, transport-cost subsidies and the overharvesting allowance.
- **Its JuMP syntax is the legacy one.** `@NLparameter`, `@NLconstraint` and `JuMP.register` are the old nonlinear interface. Current JuMP writes nonlinear constraints with plain `@constraint`. Year-to-year values can be `Parameter` variables updated before each solve, or the model can simply be rebuilt each year, which is cheap at this size.
- **Some of its packages are outdated.** It uses `IndexedTables`, which is unmaintained, and `MultiDimEquations`.

## Input data: OdsIO or YAML

OdsIO was tested on the real `data/ffsmInput.ods` (Julia 1.13.1). It reads all 14 sheets (5,947 rows in prodData, 4,848 in forData, years as `value_0…value_105`) in about 12 s the first time and 7 s after that. The keyword is `ods_readall(file; innerType="DataFrame")`. Two things to know:
- **It pulls in Python.** OdsIO wraps the Python `ezodf` library, so the first use installs a conda Python 3.12 environment through PythonCall/CondaPkg. That's fine on a workstation but heavier on a cluster.
- **It needs a cleaning layer.** Sheets come back with `Any` columns, `#` comment rows and empty trailing columns, so some code is needed to tidy them and reshape them to long format.
- **Pure-Julia alternative.** If the Python dependency is a problem, about 150 lines of Julia (unzip plus an XML parser, handling `number-columns-repeated`) would do what the C++ does today.

On YAML: it suits `settings`, `scenarios` and `gis_france.xml`. It's a poor fit for forData and prodData, which are big tables with about 106 year columns. The recommendation is to keep ODS as the file users edit, since the scenario-override workflow lives there. The model already supports cached CSVs for those two tables (`data/cachedInput/`), and the port should keep that.

## Risks

1. **Matching the C++ results.** A code comment in `Opt.cpp` says constraint order "matters for finding the equilibrium". JuMP keeps constraints in creation order, so that's reproducible. The port also needs the same starting point, the same Ipopt options (objective scaling for maximisation, tolerance, `ma27`) and the same non-smooth `mymax`. Expect agreement within a tolerance, not bit-for-bit.
2. **Stochastic scenarios won't reproduce exactly.** Julia's random-number seeding and normal sampling differ from C++'s `std::mt19937`. Scenarios like the `Sto_*` windstorm runs can only be compared statistically. Validate on deterministic scenarios first.
3. **The lookup rules.** Test them in isolation first, by comparing `gpd`/`gfd` values between the two versions before running any year.
4. **Julia start-up time.** It's negligible against an 18-minute run. It adds up when launching many processes with GNU parallel (as in `runscenarios.sh`); threads or Distributed inside one Julia process, or a compiled app/sysimage, avoid that.
5. **Linear solver.** `HSL_jll` is already available locally, so Ipopt.jl can use MA27/MA57 via the `hsllib` option.

## The GUI

The current GUI is about 2.1k lines (`MainWindow`, `MapBox`, `ScenarioSelectionWidget`). Its live features (map updates, pixel queries, pause/resume) run through Qt signals in `ThreadManager`, which every module reaches via `MTHREAD`. It offers: open a file, pick a scenario, run, live log, layer dropdown with zoom, right-click pixel query, pause and abort.

Outside the GUI, Qt use in the model code is limited to `QString`, file and directory handling, XML parsing and `QImage` map rendering.

| Option | Feasibility | Notes |
|---|---|---|
| **No GUI, command line only** | ✅ | Matches how the model is actually used. The model just needs no dependency on any GUI. |
| **Post-run viewer** (Pluto notebook or GLMakie, reading the output grids and debug pixel values) | ✅ 1–2 weeks | Fits the existing Julia output-analysis notebooks in `data/output/`. Recommended. |
| **Live GUI in GLMakie** (heatmap, layer menu, click to query, log box; model on its own thread) | ⚠️ 3–5 weeks | Possible, but basic widgets, slow first start, needs OpenGL. |
| **Gtk4.jl / QML.jl / web (Bonito, Genie)** | ⚠️ | Closest to Qt or usable remotely, but more code or fragile packaging. Not worth it for something rarely used. |

Even without a GUI, the core should call an optional "end of year" function. Then a live viewer can be added later without the model depending on it.

## Rough effort

These are estimates for one developer who knows both the model and Julia, not measurements:

| Step | Time |
|---|---|
| Data layer and GIS reading | 2–3 weeks |
| Market module and its calibration | about 2 weeks |
| Forest dynamics, management, carbon and disturbances | 3–5 weeks |
| Output CSVs in the same format, so the current post-processing scripts keep working | about 1 week |
| Checking module by module against C++ (the `debugPxValues` output and optimisation logs help) | 2–4 weeks |

That's about **2.5–4 months for the command-line version**, plus 1–2 weeks for a post-run viewer.
