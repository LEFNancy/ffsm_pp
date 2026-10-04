import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import common  # noqa: E402


def pytest_sessionfinish(session, exitstatus):
    common.cleanup()
