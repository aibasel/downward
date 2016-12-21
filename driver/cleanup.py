from itertools import count
import os

def _try_remove(f):
    try:
        os.remove(f)
    except OSError:
        return False
    return True

def cleanup_temporary_files(args):
    _try_remove("output.sas")
    _try_remove(args.plan_file)

    for i in count(1):
        if not _try_remove("%s.%s" % (args.plan_file, i)):
            break
