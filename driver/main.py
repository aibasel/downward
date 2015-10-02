# -*- coding: utf-8 -*-
from __future__ import print_function

import logging
import subprocess
import sys

from . import aliases
from . import arguments
from . import cleanup
from . import exitcodes
from . import run_components


def main():
    args = arguments.parse_args()
    logging.basicConfig(level=getattr(logging, args.log_level.upper()),
                        format="%(levelname)-8s %(message)s",
                        stream=sys.stdout)
    logging.debug("processed args: %s" % args)

    if args.show_aliases:
        aliases.show_aliases()
        sys.exit()

    if args.cleanup:
        cleanup.cleanup_temporary_files(args)
        sys.exit()

    # If validation succeeds, exit with the search component's exitcode.
    search_exitcode = None
    for component in args.components:
        try:
            if component == "translate":
                run_components.run_translate(args)
            elif component == "preprocess":
                run_components.run_preprocess(args)
            elif component == "search":
                try:
                    run_components.run_search(args)
                except subprocess.CalledProcessError as err:
                    search_exitcode = err.returncode
                    if search_exitcode not in exitcodes.EXPECTED_EXITCODES:
                        raise
            elif component == "validate":
                run_components.run_validate(args)
            else:
                assert False
        except subprocess.CalledProcessError as err:
            print(err)
            sys.exit(err.returncode)
    if search_exitcode is not None:
        sys.exit(search_exitcode)


if __name__ == "__main__":
    main()
