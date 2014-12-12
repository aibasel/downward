#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import sys


def parse_args():
    argparser = argparse.ArgumentParser()
    argparser.add_argument(
        "domain", nargs="?", help="path to domain pddl file")
    argparser.add_argument(
        "task", help="path to task pddl file")
    argparser.add_argument(
        "--relaxed", dest="generate_relaxed_task", action="store_true",
        help="output relaxed task (no delete effects)")
    argparser.add_argument(
        "--invariant-generation-max-time", default=300,
        help="max time for invariant generation (default: %(default)ds)")
    return argparser.parse_args()


def copy_args_to_module(args):
    module_dict = sys.modules[__name__].__dict__
    for key, value in vars(args).items():
        module_dict[key] = value


def setup():
    args = parse_args()
    copy_args_to_module(args)


setup()
