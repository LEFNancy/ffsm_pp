"""FFSMp: Python port of FFSM++ (Forest Sector Simulator), console version.

Run a scenario with `ffsmp.runModel("data/ffsmInput.ods", "scenarioName")` or from the command line with
`python run_ffsmp.py -s scenarioName`.
"""

import os

# The yearly market problems are small: a multi-threaded BLAS (used by the MUMPS solver of Ipopt) only uses more
# cores (and more time on short runs). Inherited by the ipopt executable; an explicit user setting is kept.
os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")
os.environ.setdefault("OMP_NUM_THREADS", "1")

from .run import runModel, main, FFSMModel  # noqa: E402
from .utils import FFSMCriticalError  # noqa: E402

__all__ = ["runModel", "main", "FFSMModel", "FFSMCriticalError"]
