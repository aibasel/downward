#!/bin/bash

set -euo pipefail

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
TESTDIR="$SCRIPTDIR/translator-package-test"
ENVDIR=.env
mkdir "$TESTDIR"
cd "$TESTDIR"
python3 -m venv "$ENVDIR"
source "$ENVDIR/bin/activate"
echo "Installing fast-downward.translate..."
echo
pip install -i https://test.pypi.org/simple/ fast-downward.translate
VERSION="$(pip3 show fast-downward.translate | awk '/^Version:/{print $2}')"

echo
echo "Testing usage as a module..."
if ! $(python3 -m fast_downward.translate ../../tests/benchmarks/gripper/domain.pddl ../../tests/benchmarks/gripper/prob01.pddl >/dev/null 2>&1); then
    echo "FAILURE!"
fi

echo
echo "Testing package as a library..."
if ! $(python3 <<'PY'
from fast_downward.translate.options import get_options, set_options
from fast_downward.translate.pddl_parser import open as pddl_open
from fast_downward.translate.normalize import normalize

set_options(["../../tests/benchmarks/gripper/domain.pddl", "../../tests/benchmarks/gripper/prob01.pddl"])
task = pddl_open(domain_filename=get_options().domain, problem_filename=get_options().problem)
normalize(task)
PY
); then
    echo "ERROR: library test failed"
fi

deactivate
cd ..
rm -rf "$TESTDIR"
echo ""
echo "Done testing fast-downward.translate version $VERSION. Please check the output for problems."
