# -*- coding: utf-8 -*-
from __future__ import print_function

import logging
import subprocess
import sys

from . import aliases
from . import arguments
from . import cleanup
from . import returncodes
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

    exitcode = None
    for component in args.components:
        try:
            if component == "translate":
                exitcode = run_components.run_translate(args)
            elif component == "search":
                exitcode = run_components.run_search(args)
            elif component == "validate":
                exitcode = run_components.run_validate(args)
            else:
                assert False
        except subprocess.CalledProcessError as err:
            print(err)
            exitcode = err.returncode
        print("{} exit code: {}".format(component, exitcode))
        if component == "translate" and exitcode != returncodes.EXIT_SUCCESS:
            break
        elif component == "search" and exitcode not in returncodes.EXPECTED_SEARCH_EXITCODES:
            # Only potentially run validate if no error was encountered
            break

        if exitcode != 0: # Stop execution as soon as one component fails.
            print("Stopping after non-zero exit code of {}".format(component))
            break
    # Exit with the exit code of the last component that ran successfully.
    # This means for example that if no plan was found, validate is not run,
    # so the return code is that of the search.
    sys.exit(exitcode)


if __name__ == "__main__":
    main()
