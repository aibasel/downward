#! /usr/bin/env python3

import os
from pathlib import Path
from shutil import copytree, ignore_patterns, rmtree
import subprocess
import sys


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT_DIR = SCRIPT_DIR.parents[1]
BUILD_DIR = SCRIPT_DIR/"build"
GENERATED_DOC_DIR = BUILD_DIR/"docs"/"documentation"/"search"

if __name__ == '__main__':
    try:
        DOWNWARD_MARKDOWN = os.environ['DOWNWARD_MARKDOWN']
    except KeyError as e:
        msg = f"Did you forget to set environment variable {e} (cf. README)?"
        sys.exit(msg)

    print("Copying content from the downward-markdown repository to",
          BUILD_DIR)
    try:
        copytree(DOWNWARD_MARKDOWN, BUILD_DIR,
                 ignore=ignore_patterns(".env", ".git", ".github"))
    except FileExistsError as e:
        sys.exit(e)
    print("Copying content from docs/")
    copytree(REPO_ROOT_DIR/"docs", BUILD_DIR/"docs"/"documentation")
    # we remove the search directory from the documentation (in case it is
    # there) because we will re-generate its content in the next step from the
    # actual code (binary).
    rmtree(GENERATED_DOC_DIR, ignore_errors=True)

    print("Start creating documentation of search plugins")
    out = subprocess.run(
        [REPO_ROOT_DIR/"misc"/"autodoc"/"generate-docs.py", "--outdir",
         GENERATED_DOC_DIR])
    print("Done creating documentation of search plugins")

    print("\n\n", "Done. Run 'mkdocs serve' from the build directory "
          "to see the website in the browser at http://localhost:8001/:\n",
          "cd build && mkdocs serve -a localhost:8001")
