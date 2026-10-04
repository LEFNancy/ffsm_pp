# Command line entry point of the Python port of FFSM++, e.g.:
#   .venv/bin/python run_ffsmp.py -s default        (or: .venv/bin/ffsmp -s default, python -m ffsmp -s default)
#   .venv/bin/python run_ffsmp.py -i data/ffsmInput.ods -s debug
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "src"))

from ffsmp import main  # noqa: E402

sys.exit(main())
