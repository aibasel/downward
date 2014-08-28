# -*- coding: utf-8 -*-

import resource
import sys

from . import aliases
from . import arguments
from . import run_components


def set_memory_limit():
    # We don't want Python to gobble up too much memory in case we're
    # running a portfolio, so we set a 50 MB memory limit for
    # ourselves. We set a "soft" limit, which limits ourselves, but
    # not a "hard" limit, which would prevent us from raising the
    # limit for our child processes. If an existing soft limit is
    # already lower, we leave it unchanged.

    # On the maia cluster, 20 MB have been tested to be sufficient; 18
    # MB were insufficent. With 50 MB, we should have a bit of a
    # safety net.

    ALLOWED_MEMORY = 50 * 1024 * 1024
    soft, hard = resource.getrlimit(resource.RLIMIT_AS)
    if soft == resource.RLIM_INFINITY or soft > ALLOWED_MEMORY:
        resource.setrlimit(resource.RLIMIT_AS, (ALLOWED_MEMORY, hard))


def main():
    args = arguments.parse_args()
    print "*** processed args:", args

    # TODO: Get rid of this:
    # raise SystemExit("stopping to debug arg parsing")

    if args.portfolio is not None:
        set_memory_limit()

    if args.show_aliases:
        aliases.show_aliases()
        sys.exit()

    for component in args.components:
        if component == "translate":
            run_components.run_translate(args)
        elif component == "preprocess":
            run_components.run_preprocess(args)
        elif component == "search":
            run_components.run_search(args)


    ## TODO: The old "plan" script preserved the exit code of the
    ## search component. Make sure that this one does, too, both in
    ## the case where only search is run and in the case where the
    ## whole planner is run. It is probably better to wait with this
    ## until the functionality of the driver has progressed further,
    ## though.


if __name__ == "__main__":
    # TODO: lab test case that tests all the aliases, including
    # unit/general case. Ditto for the portfolios.
    main()
