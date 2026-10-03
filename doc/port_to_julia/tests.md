# FFSMj test suite

The test suite of the Julia port (`ffsmj/test/`) has three layers:

| Layer | File | What it checks | Dependency on C++ |
|---|---|---|---|
| Unit tests and short run | `runtests.jl` | C++ compatible helpers (RNG, `std::min/max` semantics, formatting), map images, a 2-year run of the `debug` scenario against C++ values, `initialRandomShuffle` | values hard-coded from C++ runs |
| **Exact tests** | `exact_tests.jl` | every output file of every test scenario is equal to the one produced by the C++ binary, within the solver tolerance | needs `test/reference/` (C++ outputs) |
| **Behaviour tests** | `behaviour_tests.jl` | every test scenario moves the model in the expected direction, with a "reasonable" magnitude, and the accounting identities hold | none |

The exact tests document the current "exact port" state. Once the Julia model starts to evolve on its own
(bug fixes, refactoring, new features) they are expected to fail and can be disabled
(`FFSMJ_SKIP_EXACT=1`, or by removing the `include("exact_tests.jl")` line of `runtests.jl`).
The behaviour tests are meant to stay.

## Running the tests

```bash
cd ffsmj
julia --project=. -e 'using Pkg; Pkg.test()'                        # everything (~15-20 minutes)
FFSMJ_SKIP_EXACT=1 julia --project=. -e 'using Pkg; Pkg.test()'     # no C++ comparison
FFSMJ_SKIP_SCENARIOS=1 julia --project=. -e 'using Pkg; Pkg.test()' # only unit tests + short run
FFSMJ_TESTS=t_base,t_windstorm julia --project=. test/behaviour_tests.jl   # selected scenarios
FFSMJ_TESTS=t_polTrSub julia --project=. test/exact_tests.jl
```

Each scenario is run once per session (`runScenario()` in `common.jl` caches the model object) and is shared
by the exact and the behaviour tests. The runs use a temporary data folder (`ffsmj_tests_*` in the system
temp dir, about 8 MB plus outputs) built by `prepareTestDataDir()`: the model GIS data, an empty output
skeleton, the test input file, the scenario csv files and the synthetic test layers (copied from `test/data/gis/tests/`).

## Files

| File | Content |
|---|---|
| `test/data/ffsm_tests.ods` | test input file: `data/ffsmInput.ods` plus the `t_*` scenarios and their override sheets (generated) |
| `test/data/scenarios/t_settingFile.csv` | settings of the `t_settingFile` scenario (generated) |
| `test/build_test_input.jl` | generator of the two files above: **the scenario definitions are here** |
| `test/data/gis/tests/` | the synthetic GIS layers used by some scenarios (committed, fixed reference data) |
| `test/test_layers.jl` | generator of the synthetic GIS layers, run only to change them |
| `test/common.jl` | data preparation, cached scenario runs, readers and indicators shared by the tests |
| `test/make_cpp_references.jl` | runs the C++ binary on all the test scenarios and stores the gzipped outputs in `test/reference/` |
| `test/reference/<scenario>/` | C++ reference outputs (results, optimisation logs, maps, log of the run) |
| `test/compare_outputs.jl` | stand-alone comparison of the outputs of two full runs (e.g. `default` C++ vs Julia) |

### Re-generating the input file and the references

```bash
cd ffsmj
julia --project=. test/test_layers.jl                               # only to change the synthetic GIS layers
julia --project=. test/build_test_input.jl                           # after a change of data/ffsmInput.ods or of the scenarios
julia --project=. test/make_cpp_references.jl ../ffsm                 # all scenarios (C++ binary path), ~10 minutes
julia --project=. test/make_cpp_references.jl ../ffsm t_base t_mr0    # only some scenarios
```

The ODS file is written as XML, zipped with the `zip` tool and then re-saved with LibreOffice headless
(`soffice --convert-to ods`). The re-save is needed because the unzip code bundled with the C++ model only
accepts archives whose "version made by" is ≤ 2.0, which Info-ZIP on Unix and the Julia zip packages don't write.

## Design of the test scenarios

All the `t_*` scenarios share the same base: the two `debug` regions (11001, 11002), 8 simulated years
(2013–2020; 2011–2012 are the initialisation years), `outputLevel` 15 and the detailed harvest output.
Unless stated otherwise the change starts in **2015** (time series `ts(before, after)` in the builder), so
2013–2014 must be identical to the base and the effects are observed from 2016 (`Y1` in the tests).
Each scenario changes **one** thing compared to `t_base`, and almost all of them go through the override
mechanism used in the real projects (scenario-specific `settings`, `forData`, `prodData`, `forToProd`,
`pathRules` sheets, a csv setting file, or a GIS subfolder via `spatialDataSubfolder`).

### Origin: what the projects in `00_private` change

The scenarios were chosen by surveying the input files of the projects in `00_private/` (scenario sheets
compared with the master input file):

| Project | Main features used (overridden in the scenarios) | Test scenarios |
|---|---|---|
| 2015_ash | pathogen module (`usePathogenModule`, `pathRules`), product heterogeneity (`es_d`, `forToProd`), expectations (`expType*`), `mr` in forData, fixed replanting, diameter-detailed output | t_pathogen, t_substitution, t_expNone/Full, t_mrRegional, t_regFixed, t_outHR |
| 2015_oracle, 2022_sandrine | expectations, heterogeneous risk aversion, `mr`, spatial regional variance, forest area change (`forestAreaChangeMethod`, `forestChangeAreaIncrementsRel`), climate multipliers (`tp_multiplier`, `mortCoef_multiplier`), world prices (`pl` 99999), single-file output | t_exp*, t_noRA, t_mr*, t_noSpVar, t_areaRel/Abs/Decr, t_ccTable, t_worldPrices, t_outSingle |
| 2015_wdulef, 2017_bird | settings in the master sheet only (risk aversion, spatial variance, single file output) | t_noRA, t_noSpVar, t_outSingle |
| 2016_foretcc | policies (`pol_mktDirInt_s/_d`, `pol_trSub`, `pol_tcSub`, `pol_fiSub`), growth and mortality (`tp`, `mortCoef`), `shareMortalityUsableTimber`, supply elasticity `gamma_incr`, renamed scenarios | t_pol*, t_ccTable, t_elasticity, t_outHR |
| 2018_additionalDemand, 2019_tiga, 2022_streisand | `additionalDemand`, transport costs `ct`, supply policy, world prices, GIS subfolders | t_addDemand, t_addDemandShock, t_polTcSub, t_polSupplySub, t_worldPrices |
| 2018_mgm2cbal | climate multipliers and their spatial variance (`sStDev`, `freq`) by scenario, prices | t_ccTable, t_ccMaps, t_spPropVol |
| 2018_substitution, 2019_elorang | substitution elasticities `es_d`, product lists | t_substitution |
| 2019_miguelHartman | carbon price, Hartman payments (`flagHartman` EXO/ENDO), `pickling` | t_carbonPrice, t_hartmanExo, t_hartmanEndo, t_pickling |
| 2022_windstorms, 2025_windstorm_policies | windstorm maps (`additionalMortality` layers via `spatialDataSubfolder`), dead timber usable years (`forToProd` maxYears), supply elasticity to dead timber (`gamma_d_*`), transport cost subsidies | t_windstorm, t_windstormSto, t_polTcSub |
| 2025_elasticities | supply elasticities to the inventories (`gamma_*`, `gamma_d_*`) | t_elasticity |
| 2026_CableYarding | `forToProd`, transport costs | t_windstormSto (forToProd), t_polTcSub |

Some features of the model not (or rarely) used by the projects are tested anyway, because they are code
paths of the port: spatially explicit forest types, availability coefficients by forest type, natural
regeneration by probability of presence, the old volume-to-area method, the csv setting file, the new
random seed and the initial random shuffle.

### Synthetic GIS layers (`test/data/gis/tests/`)

Fixed data, committed with the tests (5.7 MB, 42 grids). They were derived once, deterministically (no random
draws), from the model grids by `test/test_layers.jl`, on the grid of the model's region map (`regLev_2`);
re-run it only to change them, then regenerate the C++ references. In the test data folder they are copied to
`gis/tests/` and selected by the scenarios with `spatialDataSubfolder` or the pathogen / spatially explicit
input folder settings:

| Folder | Layers | Used by |
|---|---|---|
| `ccMaps` | copy of `avalCoef`; `tp_multiplier#Fut_Feu##2015` = 0.8; `mortCoef_multiplier#Fut_Con##2015` = 1.5 | t_ccMaps |
| `aval` | `avalCoef` × 0.5; `avalCoef#Fut_Con##` = 0.2 | t_avalCoefFt |
| `windstorm` | `additionalMortality#Fut_Con#{20..60}#2016` = 0.4, `#Fut_Feu#{40,50,60}#2016` = 0.2 in region 11001; zero layers for 2017; no layers before 2016 (tests the `getSTData` fix) | t_windstorm, t_windstormSto |
| `pathogen` | `pathogen_pp#testpath#{2014..2020}.asc` = 1 in region 11001 | t_pathogen |
| `spExplicit` | `spInput#vol#Fut_Hetre#Fut_Feu#{1..13}.asc` = 3000 m³ per pixel in region 11001 | t_spExplicitFt |

## The exact tests

For each scenario with a reference folder (all but `t_newSeed` and `t_shuffle`, which are random by
construction), all the files produced by the C++ binary (results, detailed harvest, carbon balance,
optimisation log, ASCII grid maps, list layers) are compared line by line with the Julia ones:

- non numeric fields (keys, names, headers) must be identical, as the number of lines and fields;
- numbers must agree within `|a-b| ≤ 1e-8 + 1e-4·max(|a|,|b|)`;
- regional trade flows (`rt` rows) are degenerate (several optimal flows give the same objective) and use
  `|a-b| ≤ 1e-5 + 1e-2·max(|a|,|b|)`;
- in the optimisation logs the time stamp and the iteration count are not compared (the Ipopt path depends
  on the MUMPS version: C++ links MUMPS 5.8.2, Julia's Ipopt_jll MUMPS 5.9.0).

Solver fragility: in a first version of `t_windstorm` (stronger shock, see the `getSTData` finding below), the
2019 market problem, from exactly the same starting point (same objective at iteration 0), followed a different
Ipopt path with MUMPS 5.9 (Julia) than with MUMPS 5.8 (C++) and ended in "Restoration Failed", while C++
converged in 48 iterations. With `mu_strategy=adaptive` (or `nlp_scaling_method=none`, or `bound_push=1e-4`,
passed with `runModel(...; ipoptOptions = Dict(...))`) Julia reached the same optimum as C++. It is a numerical
fragility of the market problem under strong shocks, not a porting error. A possible improvement of the model
(not in the C++ one) is to retry a failed year with `mu_strategy=adaptive` before giving up.

## The behaviour tests

### Invariants (checked on every scenario)

For each region and year (2013–2020):

- the harvested volume equals what is allocated to the primary products: `hV = Σ_p st_fromHarv(p)`;
- with the fixed forest area method: the harvested area is all regenerated (`harvestedArea = Σ_ft regArea(ft)`),
  the forest area is constant and equals the sum of the pixel areas;
- market accounting: `st = sl + sa`, `dt = dl + da` for every product; prices finite and positive;
- volumes finite and non negative;
- at pixel level (last year): `Σ hVol_byPrd = Σ hVol = hV`.

### Expectations by scenario

The bounds are wide on purpose. They should catch a change that is ignored, applied with the wrong sign or
amplified out of proportion. The "observed" column gives the values of the current Julia model (equal to C++) for
reference (relative change vs t_base, 2016 if not stated).

| Scenario | Change | Expected behaviour (tested) | Observed |
|---|---|---|---|
| t_base | none | invariants; inventory growing; heterogeneous agents (expType, risk aversion, spatial modifiers vary across pixels); no additional mortality | |
| t_mr0 | `mr` = 0 | all regeneration replants the same forest type: same-type share of the regeneration = 1 | 1.0 |
| t_mr1 | `mr` = 1 | more forest type changes: same-type share < base in both regions | 0.34/0.53 vs 0.54/0.67 |
| t_mrTime | `mr` 0.7 → 0.1 from 2016 (time series setting) | same-type share > base and > 0.8 | 0.93/0.95 |
| t_mrRegional | forData `mr` 0.1 in 11001; regional setting 0.95 in 11002 | 11001 share up (≈ t_mrTime), 11002 share down: forData has priority over the setting, regional settings work | 0.93 / 0.56 |
| t_noRA | no risk aversion | `expectedReturns == expectedReturnsNotCorrByRa` on every pixel (base: corrected below on some pixels); allocation changes but < 20% | |
| t_expNone / t_expFull / t_expFixed | `expType(Prices)` 0 / 1 / −1 | pixel expectation types as set; regional expected returns of the base between the adaptive and the full ones; harvest changes < 2% (expectations drive investments, not the short-term market) | expReturns 187 / 222 / 259 |
| t_noSpVar | no spatial variance | all spatial modifiers = 1 | |
| t_spPropVol | spatial modifiers proportional to volumes | modifiers in [0.4, 1.6], different from the base, mean in [0.6, 1.4] | |
| t_natRegPP | natural regeneration by probability of presence | same-type share < base | 0.33/0.44 |
| t_regFixed | `regType` fixed | regeneration volumes stay at the input values every year (in the base they become endogenous from 2017–2018 for the fast growing types); harvest within 1% | |
| t_oldVol2Area | old volume-to-area method | harvested area changes, harvest volume within ±5%; the pixel forest area grows (0–2%) while the regional `forArea` stays constant (the flaw corrected by the new method) | +0.02% / +0.29% |
| t_noAvalCoef | availability coefficient not applied | available inventory `in` ≥ base every year, +0.1% to +50% | +3.2% |
| t_noDeathTimber | dead timber not usable | `in_deathTimber` = `usableDeadTimber` = 0; harvest of alive trees up (0–15%) | |
| t_ccTable | `tp_multiplier` × 0.8 (all), `mortCoef_multiplier` × 1.5 (Fut_Con) from 2015, forData | volume up (+0.5% to +30% in 2020), Fut_Con mortality up, nothing changes before 2015 | vol +5.6% |
| t_ccMaps | same through maps, only Fut_Feu growth | `getMultiplier` returns 0.8 / 1.5 from 2015 and 1 before or for the other types; Fut_Con mortality up; more Fut_Feu regeneration | Fut_Feu regArea +87% |
| t_avalCoefFt | `avalCoef` × 0.5, 0.2 for Fut_Con (maps) | hardwood available inventory × 0.35–0.65, softwood reduced even more | −50% |
| t_worldPrices | world prices × 1.3 from 2015 | more exports of primary products, fewer imports of secondary ones, harvest up (0–30%) | harvest +4.5% |
| t_polSupplySub | `pol_mktDirInt_s` 1.2 (subsidy) hardwood roundwood, 11001 | supply in 11001 up (+1% to +50%), local price down, `polBal_mktDirInt_s` < 0, supply in 11002 down | +12% / −13% |
| t_polDemandSub | `pol_mktDirInt_d` 0.8 (subsidy) hardwood sawnwood | consumption up (+1% to +50%), producer price up, `polBal_mktDirInt_d` < 0 | +10% |
| t_polTrSub | `pol_trSub` 0.8 softwood sawnwood | production up (+0.2% to +50%), softwood roundwood price up, `polBal_trSub` < 0 | +1.7% |
| t_polTcSub | `pol_tcSub` 0.5 all products | interregional trade up, unchanged before 2015 | rt +137% |
| t_polFiSub | `pol_fiSub` 0.5 towards Fut_Con | Fut_Con regeneration up (0–50%), `polBal_fiSub` < 0 | +1.1% |
| t_mktStr | `pol_mktStr_s/_d` 0.5 | secondary consumption down, harvest down 5–60% | harvest −30% |
| t_substitution | `es_d` fuelwood/fossil 0.5, hard/soft sawnwood 0.3 | fuelwood consumption changes (< 30%); hard and soft sawnwood consumption move in opposite directions | |
| t_addDemand | `additionalDemand` 0.3 Mm³ hardwood sawnwood, 11001, in 2015 then 0 (permanent shift) | consumption in 11001 up by 0.05–0.3 Mm³ every year from 2015 (less than 0.3: the price goes up), roughly constant; price up; harvest up | +0.26–0.28 Mm³, price +16% |
| t_addDemandShock | `additionalDemand` 0.3 in 2015, −0.3 in 2016, then 0 (single-year shock) | same as t_addDemand in 2015; from 2017 extra consumption less than half of t_addDemand; price below t_addDemand. *Broken (`@test_broken`)*: consumption back to the base (±0.02) by 2020 | 2016 price 308 vs 490; +0.083 Mm³ left |
| t_elasticity | `gamma_*` × 2 | the inventory grows in the base, so harvest up in 2020 (0–20%) | +3% |
| t_carbonPrice | carbon price 50 €/tCO2 (market) | harvest down (0 to −20%), inventory carbon stock up, no Hartman income | −0.3% |
| t_hartmanExo / t_hartmanEndo / t_pickling | + Hartman payments (EXO, ENDO), pickling 0.5 | pixel carbon income > 0, regional expected returns > t_carbonPrice, short-term harvest identical to t_carbonPrice; ENDO ≠ EXO; pickling raises the expected returns | expReturns 556 / 306 / 585 vs 223 |
| t_windstorm | windstorm maps in 11001, 2016 (zero layers in 2011 and 2017) | additional mortality only in 11001 and only in 2016 (1–30% of the stock); dead timber > 10× base; more dead timber supply and less harvest of alive trees in 2017; less volume in 2020; before 2016 only a small anticipation (< 1%): agents with full expectations read the future mortality layers | harvest −0.2% (2015), −53% (2017) | |
| t_windstormSto | + dead timber usable 4 years, `gamma_d` halved | dead timber still available in 2018 (> t_windstorm, > 10% of 2016); inventory carbon stock (alive + dead) up | |
| t_pathogen | pathogen in 11001 from 2014, Fut_Feu mortality +1/3/6% | invaded share > 0 in 11001 only; Fut_Feu mortality > 2× base in 11001, unchanged (±2%) in 11002 and for Fut_Con; increasing with the years of presence; less Fut_Feu volume | mortality ×5.9 |
| t_spExplicitFt | spatially explicit Fut_Hetre from Fut_Feu volumes in 11001 | Fut_Hetre volume and area > 0 in 11001 in all the mid diameter classes; in 11002 (no input maps) only young classes coming from the regeneration choices (Fut_Hetre is a valid choice there) and > 20× less volume; Fut_Feu volume lower; total area unchanged | |
| t_areaRel / t_areaAbs / t_areaDecr | forest area +1%/y, + hectares/y (master data), −0.3%/y | yearly ratio 1.01 / 0.997 (±0.1%); absolute: increasing, roughly constant increments; pixel areas follow; regeneration area up / down | |
| t_outHR | human readable output by diameter class, renamed scenario | files named after the new scenario name, region names, diameter class rows | |
| t_outSingle | single output files, selected years, ASCII maps 2016 | single `forestData.csv` with the year column, only 2013/2016/2020; maps only for 2016, consistent grid size | |
| t_settingFile | settings of t_mr1 from a csv file | results identical to t_mr1 | identical |
| t_newSeed | new random seed | different agents' draws, harvest and volume within 2% of the base | |
| t_shuffle | initial random shuffle of the layers | layers are permutations of the base ones; the model still runs consistently (invariants) | |

### Findings while writing the tests

- **Sign of the substitution elasticity** (`es_d`). In the demand equation (`Opt` mkeq6, `market.jl`) the
  demand is multiplied by `((pc·pol)/(pc_s·pol_s) / (same at t-1))^es_d`. With `es_d > 0` the demand of a product
  *increases* when it becomes relatively *more* expensive than its substitute, which is the opposite of a
  substitution effect (unless the convention is that `es_d` must be given negative). The test only checks that
  the two sawnwood products move in opposite directions. To be checked against the model documentation.
- **Additional demand is an increment, and a single-year shock is not fully withdrawn.** The demand curve is
  recalibrated each year on the previous consumption (`gg = dc_{t-1}·(pc_{t-1}·pol)^(-σ)`), which includes the
  additional demand, so `additionalDemand` is a yearly increment: a permanent shift is given once (0.3 then 0),
  a single-year shock is withdrawn with a negative value the following year (0.3, −0.3, then 0). But the
  recalibration absorbs the shock as *price-elastic* demand: the whole 2015 consumption at the 2015 (higher)
  price defines the new curve, which is scaled (~1.7× in the test) instead of shifted by 0.3. The −0.3 in 2016
  therefore doesn't restore the original curve: the 2016 price drops to 308 €/m³ (base 490) and about 0.08 Mm³
  (~18% of the base consumption) remain permanently. Recalibrating on `dc_{t-1} − additionalDemand_{t-1}`
  (keeping the additional demand outside the elastic curve, given as a level) would avoid it. Same in C++.
  Marked as `@test_broken` in t_addDemandShock.
- **Spatio-temporal layers before their first year** (`Pixel::getSTData()`), **fixed in C++ and Julia**. The
  layer for a year is the last key `≤ "par#ft#dc#year"` in the sorted layer map (`upper_bound` then one step
  back). When the searched key was smaller than *all* the keys of the map, `upper_bound` returned `begin()`, the
  step back was not done and the first key of the map was used: if that was a layer of the same series (e.g.
  `additionalMortality#Fut_Con#20#2016`, first alphabetically), the years before its first year got its values.
  In the first version of t_windstorm the 2016 windstorm also hit 2012–2015 for that forest type and diameter
  class. Now no layer is found and the default is used. The real projects (2025_windstorm_policies) give layers
  for every year from the initial year, so they were not affected. t_windstorm (no layers before 2016) and a unit
  test of `runtests.jl` check the fix.
- **png maps**: the last row and column of the images were never painted (`Layers::printBinMap`), **fixed in C++
  and Julia** (no visible change on the France grid, whose border is no-data).
- **Old volume-to-area method**: more area enters the first diameter class than what was harvested, so the
  pixel forest area grows slightly while the regional `forArea` stays constant.
- The **Hartman scenarios** don't change the short-term market (the harvest equals t_carbonPrice): the
  payments only change the forest type choice of the regeneration, whose volumes reach the market decades later.
- The ASCII grid maps written by the model have non standard header keys (`ncols:` with a colon, misspelled
  `lrows`, `xllcornel`; `Layers.cpp`), reproduced in Julia, and a blank line after the header. GIS readers
  expecting the ESRI keys (`nrows`, `xllcorner`) will not read them.
- Solver fragility under strong shocks: see the exact tests above.
