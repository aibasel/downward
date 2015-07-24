#! /usr/bin/env python

from itertools import count
import os

def try_remove(f):
    try:
        os.remove(f)
    except OSError:
        return False
    return True

try_remove("output.sas")
try_remove("output")
try_remove("sas_plan")

for i in count(1):
    if not try_remove("sas_plan.%s" % i):
        break
