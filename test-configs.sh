#! /bin/bash

set -euxo pipefail

BUILD=minimal_debug
DRIVER="./fast-downward.py --build $BUILD --search-memory-limit 2G output.sas"
H="lmcut()"
./build.py ${BUILD}
./fast-downward.py --translate ../benchmarks/depot/p01.pddl

${DRIVER} --search "astar(${H})"
${DRIVER} --search "astar(${H}), bound=200"
${DRIVER} --search "eager(single(${H}), reopen_closed=false, bound=infinity)"
${DRIVER} --search "eager(single(${H}), reopen_closed=false, bound=infinity, g_evaluator=g())"
${DRIVER} --search "eager(single(${H}), reopen_closed=true, bound=infinity, g_evaluator=g())"
${DRIVER} --search "eager(single(${H}), reopen_closed=false, bound=200)"
${DRIVER} --search "eager(single(${H}), reopen_closed=true, bound=200, g_evaluator=g())"
${DRIVER} --search "eager(single(${H}), reopen_closed=false, g_evaluator=g())"
${DRIVER} --search "eager(single(${H}), reopen_closed=true, g_evaluator=g())"
${DRIVER} --search "eager(single(${H}), reopen_closed=false)"
${DRIVER} --search "eager(single(${H}), reopen_closed=true)" || true
${DRIVER} --search "eager(single(${H}), reopen_closed=false, bound=infinity, g_evaluator=g(transform=adapt_costs(plusone)))"
${DRIVER} --search "eager_greedy([${H}])"
${DRIVER} --search "eager_greedy([${H}], g_evaluator=g())"

echo Tests passed
