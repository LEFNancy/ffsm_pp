"""EXACT TESTS: the outputs of the Python model must reproduce those of the C++ FFSM++ binary
(../ffsmj/test/reference/, produced by ../ffsmj/test/make_cpp_references.jl) within the tolerance of the market
solver.

These tests document the "exact port" state. They are expected to be dropped once the Python model evolves on
its own: set the environment variable FFSMP_SKIP_EXACT=1 to skip them.

All the output files of each scenario are compared line by line, in the same order:
- fields are separated by ';' (csv files) or by spaces (ASCII grid maps)
- non numeric fields must be identical (keys, names, file structure)
- numbers must agree within |a-b| <= ATOL + RTOL*max(|a|,|b|)
- regional trade flows (rt), being degenerate, have a looser tolerance
- in the optimisation logs the time stamp and the Ipopt iteration count are not compared
"""

import os

import pytest

import common
from common import compareScenario, selectedScenarios, NOT_EXACT, REFERENCE_DIR

SKIP = os.environ.get("FFSMP_SKIP_EXACT", "0") == "1"


@pytest.mark.skipif(SKIP, reason="exact tests skipped (FFSMP_SKIP_EXACT=1)")
@pytest.mark.parametrize("sc", [s for s in common.testScenarios() if s not in NOT_EXACT])
def test_exact(sc):
    if sc not in selectedScenarios():
        pytest.skip("scenario not selected")
    if not os.path.isdir(os.path.join(REFERENCE_DIR, sc)):
        pytest.skip("no C++ reference")
    bad = [(f, n, msgs) for f, n, msgs in compareScenario(sc) if n]
    assert not bad, "\n".join(f"{f}: {n} differences: {msgs}" for f, n, msgs in bad)
