# FFSM++ → Julia port: status

*Status as of 2026-10-02. The Julia code lives in `ffsmj/`. Validated against the C++ binary built
from commit `7884375`.*

This document records what the Julia port (`ffsmj/`, package `FFSMj`) covers, how it was checked
against the C++ model, and above all **what has not been ported yet**, or has been ported but never
exercised. The feasibility study that preceded the port is in `feasibility.md` in this folder. Usage
instructions are in `ffsmj/README.md`.

## 1. Summary

- **What runs.** The console model (`ffsm -c`) runs entirely in Julia, from the same input files:
  the ODS spreadsheet, the GIS XML file and the ASCII grids. It writes the same output files.
- **How it was checked.** It reproduces the C++ results within the Ipopt tolerance on four
  scenarios: `debug`, `default`, and `Windstorm_Ref` and `Sto_M` from the 2025 windstorm project.
  The years before the first optimisation, the random per-pixel values and all the discrete
  forest-owner choices are identical to C++.
- **Speed.** It is 2.5–6× faster than C++ and, on the windstorm scenarios, uses less than half the
  memory.
- **What is missing.**
  - Not ported: the GUI and the non-spatial model (`usePixelData = false`).
  - Ported with different random draws: the `initialRandomShuffle` option (section 3.1).
  - Ported but never run: several modules and options that none of the available input files turn
    on (section 4).

## 2. Mapping of the C++ sources

| C++ file(s) | Lines | Status in Julia |
|---|---|---|
| `ModelCoreSpatial` | 2,916 | **Ported** (`core.jl`), except `printDebugInitRegionalValues()` (a debug helper that ends the program) |
| `ModelData`, `ModelRegion` | 2,437 | **Ported** (`modeldata.jl`, `datamap.jl`) |
| `Opt` | 1,874 | **Rewritten** with JuMP (`market.jl`) as an algebraic model: variables indexed by region and product (`pl[r, p]`, `rt[r, p, rTo]`, ...) and one family of constraints per market equation, with the C++ names (`mkeq2[r in R, s in SEC]`, ...). Same variables and constraints, in the same order, same bounds, start point, objective and Ipopt options. The ADOL-C taping, sparsity patterns and index bookkeeping are no longer needed. |
| `Gis`, `Layers`, `Pixel` | 1,845 | **Ported** (`gis.jl`), except the unused helpers listed in section 3.4 |
| `Output` | 1,215 | **Ported** (`output.jl`), except the non-spatial debug output (section 3.2) |
| `Carbon` | 349 | **Ported** (`carbon.jl`) |
| `Init`, `Scheduler`, `MainProgram`, `main` | 512 | **Ported** (`run.jl`), console mode only |
| `BaseClass` | 664 | **Ported** where used (`utils.jl`), plus C-compatible helpers: `cmin`/`cmax`, glibc `pow`/`exp`/`log`, `%g` formatting |
| `<random>` usage | — | **Bit-exact replica** of `std::mt19937`, `generate_canonical` and libstdc++ `normal_distribution` (`rng.jl`) |
| `InputNode` (QtXml), `unzip`/`zip` | 2,862 | **Replaced** by EzXML and ZipArchives (`odsreader.jl`), with the same traversal rules |
| `anyoption` | 1,445 | **Replaced** by a small argument parser in `run.jl` |
| `ModelCore` (non-spatial core) | 938 | **Not ported** |
| `ThreadManager`, `MainWindow`, `MapBox`, `ScenarioSelectionWidget`, `ui_MainWindow.h`, `resources.qrc` | ~1,700 | **Not ported** (GUI) |
| `Sandbox`, `Adolc_debugtest`, `Ipopt_nlp_problem_debugtest`, `zipExampleUsage`, `CommonLib` | ~2,900 | **Not ported** (developer tests, examples, empty file) |

## 3. Not ported

### 3.1 Features and options

| Item | What happens in Julia | Notes |
|---|---|---|
| **GUI** (map viewer, layer selection, pixel query, live log, pause/resume, scenario dialog, "open results" button) | Not available. `-c` is accepted and ignored. | All the C++ GUI calls (`updateImage`, `refreshGUI`, tree viewer…) are dropped. Only the `Update image …` debug messages are kept, so the logs stay comparable. |
| **Non-spatial model** (`ModelCore`, `usePixelData = false`) | Critical error at start-up | 938 lines. In the C++ code it is an older, regional-only variant of `ModelCoreSpatial`. |
| **`initialRandomShuffle = true`** | **Ported, but with Julia's random draws** (`Random.shuffle!` with a `Xoshiro` generator), so the shuffled maps differ from the C++ ones | A sensitivity-only option. After each grid file is read, its values are randomly reassigned among the pixels with data (the no-data mask is kept). As in the C++ code it applies to **every** layer read from file, not only to the integer ones as the setting description says. This includes the region layers `regLev_1`/`regLev_2`, so pixels also change region. As in C++, the permutation is the same at every run unless `newRandomSeed = true`. Tested: one-year run in `Pkg.test()` and a full 100-year `debug` run; not compared with C++, by design. |
| **Probabilistic map-code reclassification** (`Layers::filterExogenousDataset`, `reclassRulesVector`) | Not ported (identity: the value is kept) | Dead code inherited from RegMAS, where a pixel held a single category and the `<reclassRule>` rules of the GIS XML file split a category at random, pixel by pixel. FFSM replaced it in June 2012 (`e8db114`) with the deterministic forest-type reclassification (`reclRules` sheet, `Gis::applyForestReclassification`, ported), where each pixel holds an area for every forest type. The XML reading of the old rules was removed in 2016 (`ef82f89`), so the rule vector is never filled and the C++ function always returns its input. |
| **`newRandomSeed = true`** | Supported, but with a different seed | C++ seeds with `time(0)+getpid()`. Julia uses the same formula, but the run time differs, so runs aren't reproducible between versions. They aren't reproducible within C++ either. |

### 3.2 Outputs

| Item | Status |
|---|---|
| `debugs/debugOut_*.csv` (`Output::initDebugOutput`/`printDebugOutput`) | Not ported. In the C++ code it is written only by the non-spatial model. |
| png maps (`outputLevel` ≥ 20) | Ported, written by a small built-in PNG writer (zlib from `Zlib_jll`) instead of Qt's `QImage::save`. The files are **not byte-identical**: there is no Qt metadata and the compression differs. The pixels are identical to the C++ maps. |
| Order of the layers in `maps/floatListLayers`, `maps/integerListLayers` and in map printing | It can differ. C++ adds the spatial-data layers in `readdir()` (filesystem) order, Julia in alphabetical order. The content is the same. |
| Console log | Same messages and prefixes, except: the banner (no compile date or commit), `Correctly deleted old temporary data` (no temporary unzip folder in Julia), the Ipopt banner/version, and the Ipopt iteration counts (section 5). |
| `optimisationLogs` | Same format. Time stamps and iteration counts differ (section 5). |

### 3.3 Developer and debug helpers

None of these is called on the console path of the C++ model:

- `ModelCoreSpatial::printDebugInitRegionalValues()` (prints and calls `exit(0)`).
- `Sandbox` (`basicTest`, `fullTest`, `testIpopt`, `testAdolc`, thread tests…); `Opt::tempDebug`,
  `calculateSparsityPatternJ/H`; the ADOL-C tape file `ADOLC-Taylors_*.tap` is no longer produced.
- `ModelData::addSetting`, `getScenarioIndex`, `getForTypeCounter`, `getFilenameByType`,
  `regSName2RegId`, `unpackKey*` (only used internally, replaced), `loadDataFromCache` (commented out
  in C++).

### 3.4 Unused GIS/pixel helpers

These are not ported: `Gis::countItems`, `getAllPlotsByValue` (both overloads), `getRandomPlotByValue`,
`getDistance`, `getLayerNames`, `getLayerPointers`, `printDebugValues`, `sub2realID`, `swap`,
`unpack`; `Layers::countMyPixels`, `getCategory`; `Pixel::getPixelsAtDistLevel`,
`correctInputMultiplier`, `setPastRegArea` (a `TODO` that raises a critical error in C++);
`ModelRegion::getVolumes` (empty stubs in C++).

`Gis::getAllPlots()` is replaced by its observable effect. In C++ it is called before the pixels are
assigned to the regions, and its `if(masked = false)` assignment means it always returns an empty set,
so the `pixelIds` layer stays filled with no-value and a `We can't find any free plot.` warning is
printed. Julia does exactly that.

## 4. Features validated only by the test scenarios

The features below are not turned on by the large validation runs of section 7. They are exercised by the
test scenarios of `ffsmj/test/data/ffsm_tests.ods` (2 regions, 8 years), whose outputs match the C++ ones
(`test/exact_tests.jl`; see [tests.md](tests.md)):

| Feature | Setting / input | Test scenario |
|---|---|---|
| Pathogen module, pathogen rules overridden by scenario | `usePathogenModule = true`, `pathTable`, `pathogen_pp#…` layers | t_pathogen |
| Hartman carbon payments, pickling | `flagHartman = EXO` / `ENDO`, `pickling` | t_hartmanExo, t_hartmanEndo, t_pickling |
| Spatially explicit forest types | `useSpExplicitForestTypes = true`, `memType = 3`, `spInput#vol#…` layers | t_spExplicitFt |
| Forest area changes | `forestAreaChangeMethod = relative` / `absolute` | t_areaRel, t_areaAbs, t_areaDecr |
| Natural regeneration by probability of presence | `natRegAllocation = pp` | t_natRegPP |
| Old volume-to-area conversion | `oldVol2AreaMethod = true` | t_oldVol2Area |
| Spatial multipliers proportional to volumes | `useSpatialVarPropToVol = true` | t_spPropVol |
| Expectations fixed at the initial year | `expType = -1` | t_expFixed |
| Scenario settings from a csv file | `settingFile` column of `scenarios` | t_settingFile |
| Output variants | `outputSingleFile`, `outputHumanReadable`, `forestDiamDetailedOutput`, `outStepRange = -1`, `overridenScenarioName`, `mapOutputFormat = 2` | t_outHR, t_outSingle |
| Climate multipliers from maps, availability by forest type | `modifiersFromTable`, `spatialDataSubfolder` | t_ccMaps, t_avalCoefFt |
| Cached input tables, a file without extension in the spatial data folder | `cachedInput/forData.csv` and `prodData.csv` | t_base (behaviour tests: same results as from the ODS sheets; also checked once with the C++ binary) |

Still never run:

| Feature | Setting / input | Code |
|---|---|---|
| HSL linear solvers | `linearSolver = ma27/ma57` | Needs `--hsllib <path>` (e.g. from `HSL_jll`) |

These code paths **are** exercised by the validation scenarios: the market module including
substitution elasticities and policy terms, the death-timber inventory and its allocation to supply
(`forToProd` `maxYears` 1 and 4), additional mortality maps (3,121 layers in the windstorm scenarios),
scenario overrides of settings/prodData/forToProd, regional overriding, debug mode, heterogeneous risk
aversion, stochastic expectations and spatial variance, the carbon balance, detailed harvest output,
debug pixel values, and ASCII/png maps.

## 5. Differences that remain by design

- **Market solution.** Same NLP, but solved by Ipopt.jl's Ipopt/MUMPS with JuMP derivatives instead of
  ADOL-C. The constraint rows may also be ordered differently. The
  equilibrium therefore agrees within the tolerance (`tol = 1e-6`), not bit-for-bit, and iteration
  counts differ by a few units.
- **Small, flat sub-markets.** The Corsican pulpwood market (about 0.0025 Mm³) is only resolved to
  about 1e-4–5e-3 relative at this tolerance, in **both** versions. Single years can then differ
  noticeably (2098 in `default`, 2095 before the rewrite of the market module; 2045 in `Sto_M`), before re-converging the next year. Re-solving with
  `tol = 1e-11` shows that neither default-tolerance solution is exact there. A tighter tolerance can
  be tested with `runModel(...; ipoptOptions = Dict("tol" => 1e-9))`.
- **Regional trade flows (`rt`)** are degenerate (several equivalent optima) and differ most in
  relative terms. Over 100 years of `default` the totals are identical and the sum of absolute
  differences is 0.0009 Mm³.

## 6. Deliberate deviations from the C++ code

The port reproduces the C++ behaviour, including its quirks (listed in `ffsmj/README.md`). The
exception is:

| Case | C++ | Julia |
|---|---|---|
| `initialRandomShuffle = true` | Shuffles the layers with `std::random_shuffle`/`rand()` | Shuffles the layers with Julia's `shuffle!`, so the permutations differ (section 3.1) |

Bugs found during the port and fixed in **both** versions:

| Case | Before | Now |
|---|---|---|
| png maps (and GUI map images): last row and column | Never painted, so always white (the loops in `Layers::printBinMap` and `Gis::updateImage` stopped one short) | The whole map is painted. No visible change on the France grid, whose border cells are no-data |
| Spatio-temporal layer of a year before the first layer of its series (`Pixel::getSTData`) | When the searched key was smaller than all the layer names, `upper_bound` returned `begin()` and the first layer of the map was used: the first series in alphabetical order (e.g. `additionalMortality#…`) applied its first layer also to the previous years | No layer: the default value is returned |
| `cachedInput/*.csv` | The base directory was prepended twice, so the files were never found and the tables were empty; and `prodData.csv` was looked for whenever `forData.csv` existed | Each table is read from its own csv file when it exists |
| A file without extension in a spatial-data folder (with an extension filter) | `std::out_of_range` crash in `getFilenamesByDir` | The file is ignored |
| `getPastRegArea` for a year that is not registered | Error message, then an undefined return value | Error message, returns 0 |
| `setTimedData` for a year before the initial year | Infinite loop (unsigned arithmetic) | Critical error |
| `getTimedData` for a year before the initial year | Returned the *last* value (unsigned comparison). Never happens in the current model (checked on all the test scenarios and `debug`/`default`) | Returns the first value, with a warning |

## 7. Validation done

The comparisons use `ffsmj/test/compare_outputs.jl`, on the same inputs:

| Scenario | Years | Files compared | Result |
|---|---|---|---|
| `debug` | 2013–2112 | forest/product data, carbon balance, 3.5 M lines of debug pixel values, 47 ASCII maps, 42 png | Objective identical every year (6 digits). Discrete choices (`optDc`, `optFtChosen`, `optDcChosen`) identical. Values within ~1e-5 (last printed digit). 40/47 maps byte-identical, png pixel-identical. |
| `default` | 2013–2112 | forest/product data, carbon balance | Within 2.2e-5 until 2097, up to ~3e-3 in 2098 on the Corsican market (section 5), within 6e-5 afterwards |
| `Windstorm_Ref` | 2013–2050 | + detailed harvest by product | Within 1e-5, except some trade flows |
| `Sto_M` | 2013–2050 | + detailed harvest by product | Within 1e-5, except in 2045 (section 5) |

`ffsmj/test/runtests.jl` (`Pkg.test()`) checks the RNG replica against libstdc++ values, a few C++
semantics (`minmax_element` ties, conversions, prefix look-ups), a short `debug` run against C++
reference values, and `initialRandomShuffle` (the layers are permutations of the original ones, with
the same no-data mask and the same permutation at every run). It then runs the 46 test scenarios of
`test/data/ffsm_tests.ods`: exact comparison with the C++ outputs and behaviour tests (see
[tests.md](tests.md)).

## 8. Suggested next steps

1. Validate on full-size runs the features of section 4 that current projects use (the test scenarios
   only cover 2 regions and 8 years). Look at the issues of the model found by the tests (tests.md).
2. Replace `ffsmj/data/` with a symlink to `../data` once the two versions share the inputs.
3. Decide whether the solver tolerance should be tightened. This affects the C++ model as well
   (section 5).
4. If a GUI is ever needed, write it as a separate post-run viewer (see `feasibility.md`), keeping the
   model free of GUI dependencies.
