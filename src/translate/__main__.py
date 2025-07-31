import os
import signal
import sys
import traceback

from translate import pddl_parser
from translate.main import main

## For a full list of exit codes, please see driver/returncodes.py. Here,
## we only list codes that are used by the translator component of the planner.
TRANSLATE_OUT_OF_MEMORY = 20
TRANSLATE_OUT_OF_TIME = 21
TRANSLATE_INPUT_ERROR = 31

def handle_sigxcpu(signum, stackframe):
    print()
    print("Translator hit the time limit")
    # sys.exit() is not safe to be called from within signal handlers, but
    # os._exit() is.
    os._exit(TRANSLATE_OUT_OF_TIME)

if __name__ == "__main__":
    from translate.options import set_options

    set_options() # use command line options
    try:
        signal.signal(signal.SIGXCPU, handle_sigxcpu)
    except AttributeError:
        print("Warning! SIGXCPU is not available on your platform. "
              "This means that the planner cannot be gracefully terminated "
              "when using a time limit, which, however, is probably "
              "supported on your platform anyway.")
    try:
        # Reserve about 10 MB of emergency memory.
        # https://stackoverflow.com/questions/19469608/
        emergency_memory = b"x" * 10**7
        main()
    except MemoryError:
        del emergency_memory
        print()
        print("Translator ran out of memory, traceback:")
        print("=" * 79)
        traceback.print_exc(file=sys.stdout)
        print("=" * 79)
        sys.exit(TRANSLATE_OUT_OF_MEMORY)
    except pddl_parser.ParseError as e:
        print(e)
        sys.exit(TRANSLATE_INPUT_ERROR)
    main()
