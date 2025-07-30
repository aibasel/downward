# IPC planners

This page describes how to run any of the planner configurations based
on Fast Downward that participated in IPC 2011 and the two Fast Downward
Stone Soup portfolios from IPC 2018.

## What is the relationship between "Fast Downward" and the IPC 2011 planners based on Fast Downward?

All the following planners from IPC 2011 (called "IPC configurations"
in the following) used exactly the same code, a snapshot from the Fast
Downward repository. They only differ in command-line options for the
search component of the planner.

-   satisficing track:

   -   Fast Downward Autotune 1, satisficing version (`seq-sat-fd-autotune-1`)
   -   Fast Downward Autotune 2, satisficing version (`seq-sat-fd-autotune-2`)
   -   \*Fast Downward Stone Soup 1, satisficing version (`seq-sat-fdss-1`)
   -   \*Fast Downward Stone Soup 2, satisficing version (`seq-sat-fdss-2`)
    -   LAMA 2011 (`seq-sat-lama-2011`)

-   optimization track:
   -   BJOLP (`seq-opt-bjolp`)
   -   Fast Downward Autotune, optimizing version (`seq-opt-fd-autotune`)
   -   \*Fast Downward Stone Soup 1, optimizing version (`seq-opt-fdss-1`)
   -   \*Fast Downward Stone Soup 2, optimizing version (`seq-opt-fdss-2`)
   -   LM-Cut (`seq-opt-lmcut`)
   -   *Merge and Shrink (`seq-opt-merge-and-shrink`)
   -   Selective Max (`seq-opt-selmax`)

The planners marked with a star (\*) are portfolio configurations, which
are a bit special because they use hard-coded time limits. See below.

!!! note
    **Note on running LAMA:** As of 2011, LAMA has been merged back into the
    Fast Downward codebase. "LAMA 2011", the version of LAMA that participated
    in IPC 2011, is Fast Downward with a particular set of command-line arguments.
    Since LAMA 2011 greatly outperformed LAMA 2008 in the competition, we strongly
    encourage you to use the current Fast Downward code when evaluating your
    planner against LAMA.

## IPC 2018 Fast Downward Stone Soup planners

We describe the two portfolios in this [planner
abstract](https://ai.dmi.unibas.ch/papers/seipp-roeger-ipc2018.pdf).
The two portfolios use the same configurations but the cost-bounded
version stops after finding the first plan that's cheaper than the
given cost bound.

-   satisficing track:
        `./fast-downward.py --alias seq-sat-fdss-2018 --overall-time-limit 30m ../benchmarks/depot/p01.pddl`
-   cost-bounded track:
        `./fast-downward.py --portfolio-single-plan --portfolio-bound=20 --alias seq-sat-fdss-2018 --overall-time-limit 30m ../benchmarks/depot/p01.pddl`

The portfolios are available in several versions:

-   The version that was used in IPC 2018: [satisficing
    track](https://bitbucket.org/ipc2018-classical/team45/src/ipc-2018-seq-sat/)
    and [cost-bounded
    track](https://bitbucket.org/ipc2018-classical/team45/src/ipc-2018-seq-cbo/).
    Both repositories contain Singularity files for easier compilation.
-   Released version from the [Fast Downward releases](https://www.fast-downward.org/latest/releases/)
-   Latest revision from [Git repository](https://github.com/aibasel/downward/).

## How do I run an IPC configuration?

The `src/fast-downward.py` script provides short aliases for the IPC
configurations. To run e.g.  the LAMA-2011 configuration on the first gripper
task run

    ./fast-downward.py --alias seq-sat-lama-2011 misc/tests/benchmarks/gripper/prob01.pddl

and the correct parameter settings will automatically be set. To see all
available aliases run

    ./fast-downward.py --show-aliases

If you are interested in the actual parameter settings, look inside the

    src/driver/aliases.py

module.

## Using time limits other than 30 minutes

The portfolio configurations are intimately tied to the competition time
limit of 30 minutes. If you use a different time limit, you cannot use
these planner configurations out of the box.

Please also note that *all* planner configurations have an internal time
limit of 5 minutes for the invariant synthesis part of the translator.
This is generous for all but very few planning tasks, but still it makes
sense to adapt this value if you're running the overall planner with a
different overall timeout from the usual 30 minutes, especially if it is
a significantly lower time limit.

## Which code version of the IPC 2011 planners should I use?

The two main options are

-   [the snapshot used at IPC 2011](../../files/ipc-2011-submission.tar.gz)
-   the most current version of the main branch of the repository

We usually recommend using the newest code from the repository since we
tend to fix bugs every now and then (but of course, we also introduce
new ones...). If you do a proper experiment and performance in some
domain looks worse than what you'd expect from our papers or the IPC
results, we're very happy to be notified since this may be an
indication that we introduced a bug.

The tarball linked above is identical to the code that was run at IPC
2011 except that it uses 32-bit mode rather than 64-bit mode. For an
explanation of which of these two modes you want and how to set it,
please check the [planner usage](planner-usage.md) page.
