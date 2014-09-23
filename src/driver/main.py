# -*- coding: utf-8 -*-
from __future__ import print_function

import resource
import subprocess
import sys

from . import aliases
from . import arguments
from . import run_components

# TODO: lab test case that tests all the aliases, including
# unit/general case. Ditto for the portfolios.


def main():
    args = arguments.parse_args()
    print("*** processed args: %s" % args)

    # TODO: Get rid of this:
    # raise SystemExit("stopping to debug arg parsing")

    if args.show_aliases:
        aliases.show_aliases()
        sys.exit()

    try:
        for component in args.components:
            if component == "translate":
                run_components.run_translate(args)
            elif component == "preprocess":
                run_components.run_preprocess(args)
            elif component == "search":
                run_components.run_search(args)
            else:
                assert False
    except subprocess.CalledProcessError as err:
        print("Command \"%s\" returned exitcode %d" %
              (" ".join(err.cmd), err.returncode))
        sys.exit(err.returncode)


if __name__ == "__main__":
    main()
