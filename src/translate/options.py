import argparse
import sys

import arguments

try:
    import argcomplete
    HAS_ARGCOMPLETE = True
except ImportError:
    HAS_ARGCOMPLETE = False


def copy_args_to_module(args):
    module_dict = sys.modules[__name__].__dict__
    for key, value in vars(args).items():
        module_dict[key] = value


def setup():
    argparser = argparse.ArgumentParser()
    arguments.add_args(argparser)

    if HAS_ARGCOMPLETE:
        argcomplete.autocomplete(argparser)

    args = argparser.parse_args()
    copy_args_to_module(args)


setup()
