# -*- coding: utf-8 -*-
from __future__ import print_function

import logging
import subprocess
import sys

from . import aliases
from . import arguments
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
        print(err)
        sys.exit(err.returncode)


if __name__ == "__main__":
    main()
