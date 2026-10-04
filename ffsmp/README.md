# FFSMp — Python port of FFSM++ (console version)

FFSMp is a port of the FFSM++ forest sector model to Python, from the Julia port (`../ffsmj`, the main port). It
runs the same input files and writes the same output files as the C++ binary in console mode (`ffsm -c`); there is
no GUI. The documentation of the port (design, validation against C++, performance, tests) is in
[../doc/port_to_python/](../doc/port_to_python/port_status.md).

## Installing and running

From this folder:

```bash
python3 -m venv .venv                       # first time only
.venv/bin/pip install -e ".[test]"          # pyomo and pytest
.venv/bin/python run_ffsmp.py -s default    # scenario "default" of data/ffsmInput.ods
.venv/bin/python run_ffsmp.py -i data/ffsmInput.ods -s debug
```

Python ≥ 3.10. The only Python dependency is [Pyomo](https://www.pyomo.org) (the market model). Pyomo needs the
`ipopt` executable on the `PATH`: from the system packages (e.g. `coinor-ipopt`), from conda-forge
(`conda install -c conda-forge ipopt`) or from the IDAES extensions (`idaes get-extensions`).

The same is available as `.venv/bin/ffsmp -s default` (installed by pip) or `python -m ffsmp -s default`. The
options are those of the C++ binary: `-i` input file (relative to the current directory), `-s` scenario (default:
the first one of the `scenarios` sheet), `-c` (accepted and ignored) and `-h`. `--hsllib <path>` gives Ipopt the
path to the HSL library, for the `ma27`/`ma57` linear solvers.

From Python: `import ffsmp; m = ffsmp.runModel("data/ffsmInput.ods", "default")`. The returned object holds the
whole model state. As in the Julia port, `settings={"simulationYears": 5}` overrides (world-level) settings and
`ipoptOptions={"tol": 1e-9}` passes further options to Ipopt.

The base directory is the folder of the input file, and the outputs go to the folder given by the `outputDirname`
setting (`data/output/`). `data/` is a copy of the input files of the Julia port.

## Tests

```bash
.venv/bin/python -m pytest                                   # everything (~22 minutes)
FFSMP_SKIP_EXACT=1 .venv/bin/python -m pytest                # without the C++ reproduction tests
FFSMP_TESTS=t_base,t_pathogen .venv/bin/python -m pytest     # only some scenarios
.venv/bin/python -m pytest tests/test_units.py               # only the unit tests
```

They use the test input file and the C++ reference outputs of the Julia port (`../ffsmj/test/`). See
[../doc/port_to_python/port_status.md](../doc/port_to_python/port_status.md#7-tests).
