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
# Translator

Fast Downward's PDDL to SAS^+ translator can be called through Fast Downward's driver script

```
./fast-downward.py --translate [domain] problem [--translate-options OPTIONS]
```

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


def parser_to_markdown(parser: argparse.ArgumentParser):
    lines = [INTRO_TEXT]
    lines.append("## Options")
    lines.append("")

    for action in parser._actions:
        if action.option_strings:
            name = ", ".join(f"`{s}`" for s in action.option_strings)
        else:
            name = f"`{action.dest}`"
        lines.append(f"### {name}")
        lines.append(action.help)
        lines.append("")
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
