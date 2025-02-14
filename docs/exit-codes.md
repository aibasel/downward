# Exit Codes

Fast Downward returns the following positive exit codes, listing the
names as used in
[src/search/utils/system.h](https://github.com/aibasel/downward/blob/main/src/search/utils/system.h)
and
[driver/returncodes.py](https://github.com/aibasel/downward/blob/main/driver/returncodes.py).
If it gets killed by a specific signal, it returns the signal as exit
code, except for SIGXCPU which we intercept. The exit codes we defined
are grouped in four blocks.

The first block (0-9) represents successful termination of a component
which does not prevent the execution of further components.

| **Code** | **Name** | **Meaning** |
| -------- | -------- | ----------- |
| 0 | SUCCESS | All run components successfully terminated (translator: completed, search: found a plan, validate: validated a plan) |
| 1 | SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY | Only returned by portfolios: at least one plan was found and another component ran out of memory. |
| 2 | SEARCH_PLAN_FOUND_AND_OUT_OF_TIME | Only returned by portfolios: at least one plan was found and another component ran out of time. |
| 3 | SEARCH_PLAN_FOUND_AND_OUT_OF_MEMORY_AND_TIME | Only returned by portfolios: at least one plan was found, another component ran out of memory, and yet another one ran out of time. |

The second block (10-19) represents unsuccessful, but error-free
termination which prevents the execution of further components.

| **Code** | **Name** | **Meaning** |
| -------- | -------- | ----------- |
| 10 | TRANSLATE_UNSOLVABLE | Translator proved task to be unsolvable. Currently not used |
| 11 | SEARCH_UNSOLVABLE | Task is provably unsolvable with current bound. Currently only used by hillclimbing search. See also [issue377](http://issues.fast-downward.org/issue377). |
| 12 | SEARCH_UNSOLVED_INCOMPLETE | Search ended without finding a solution. |

The third block (20-29) represents expected failures which prevent the
execution of further components.

| **Code** | **Name** | **Meaning** |
| -------- | -------- | ----------- |
| 20 | TRANSLATE_OUT_OF_MEMORY | Memory exhausted. |
| 21 | TRANSLATE_OUT_OF_TIME | Time exhausted. Not supported on Windows because we use SIGXCPU to kill the planner. |
| 22 | SEARCH_OUT_OF_MEMORY | Memory exhausted. |
| 23 | SEARCH_OUT_OF_TIME | Timeout occurred. Not supported on Windows because we use SIGXCPU to kill the planner. |
| 24 | SEARCH_OUT_OF_MEMORY_AND_TIME | Only returned by portfolios: one component ran out of memory and another one out of time. |

The fourth block (30-39) represents unrecoverable failures which prevent
the execution of further components.

| **Code** | **Name** | **Meaning** |
| -------- | -------- | ----------- |
| 30 | TRANSLATE_CRITICAL_ERROR | Critical error: something went wrong (e.g. translator bug, but also malformed PDDL input). |
| 31 | TRANSLATE_INPUT_ERROR | Usage error: wrong command line options or invalid PDDL inputs |
| 32 | SEARCH_CRITICAL_ERROR | Something went wrong that should not have gone wrong (e.g. planner bug). |
| 33 | SEARCH_INPUT_ERROR | Wrong command line options or SAS+ file. |
| 34 | SEARCH_UNSUPPORTED | Requested unsupported feature. |
| 35 | DRIVER_CRITICAL_ERROR | Something went wrong in the driver (e.g. failed setting resource limits, ill-defined portfolio, complete plan generated after an incomplete one). |
| 36 | DRIVER_INPUT_ERROR | Usage error: wrong or missing command line options, including (implicitly) specifying non-existing paths (e.g. for input files or build directory). |
| 37 | DRIVER_UNSUPPORTED | Requested unsupported feature (e.g.  limiting memory on macOS). |
