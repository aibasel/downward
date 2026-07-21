#! /usr/bin/env python3

import argparse
import logging
from pathlib import Path
import subprocess
import sys

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT_DIR = SCRIPT_DIR.parents[1]
BUILD = "release"
FILENAME = "translator-usage.md"

INTRO_TEXT = """
# Translator Usage

Fast Downward's PDDL to SAS^+ translator can be called through Fast Downward's driver script

```
./fast-downward.py --translate [domain] problem [--translate-options OPTIONS]
```

## Options

"""

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", default=f"{REPO_ROOT_DIR}/docs")
    return parser.parse_args()


def build_translator():
    subprocess.check_call([sys.executable, "build.py", BUILD, "translate"], cwd=REPO_ROOT_DIR)


def import_argparser_from_translator():
    sys.path.insert(0, str(REPO_ROOT_DIR / "builds" / BUILD / "bin"))
    from translate import options
    return options.get_arg_parser()


def action_to_markdown(parser: argparse.ArgumentParser, action: argparse.Action):
    if action.option_strings:
        names = []
        for opt in action.option_strings:
            if action.choices is not None:
                choice_str = ",".join(map(str, action.choices))
                names.append(f"`{opt} {{{choice_str}}}`")
            elif action.metavar is not None:
                names.append(f"`{opt} {action.metavar}`")
            elif action.nargs != 0:
                metavar = action.dest.upper()
                names.append(f"`{opt} {metavar}`")
            else:
                names.append(f"`{opt}`")
        name = ", ".join(names)
    else:
        name = f"`{action.dest}`"
        # Expand %(default)s, %(choices)s, etc.

    help_text = action.help or ""
    if help_text is not argparse.SUPPRESS:
        params = {
            "default": action.default,
            "prog": parser.prog,
            "type": action.type,
            "choices": action.choices,
            "metavar": action.metavar,
            "dest": action.dest,
        }
        try:
            help_text = help_text % params
        except Exception:
            pass

    return f"### {name}\n{help_text}\n"


def parser_to_markdown(parser: argparse.ArgumentParser):
    lines = [INTRO_TEXT]
    for action in parser._actions:
        lines.append(action_to_markdown(parser, action))
    return "\n".join(lines)


def build_docs(path: Path):
    parser = import_argparser_from_translator()
    path.write_text(parser_to_markdown(parser))


if __name__ == '__main__':
    args = parse_args()
    logging.info("building translator...")
    build_translator()
    logging.info("building documentation...")
    outdir = SCRIPT_DIR / args.outdir
    outdir.mkdir(parents=True, exist_ok=True)
    outpath = outdir / FILENAME
    if outpath.exists():
        sys.exit(f"{outpath} already exists.")
    build_docs(outpath)
