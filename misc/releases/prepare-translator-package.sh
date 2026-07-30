#!/bin/bash

## Assumptions:
##  * The repository containing this script is on the revision we want to release.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: ./prepare-translator-package.sh 19.06"
    exit 1
fi
MAJOR=$1

#if [[ ! "$MAJOR" =~ ^[1-9][0-9]\.[0-9][0-9]$ ]]; then
#    echo "Unrecognized version number '$MAJOR'. Expected the format YY.MM (e.g. 19.06)."
#    exit 1
#fi

# Verify that repository is clean
if [[ -n $(git status --porcelain) ]]; then
    echo "Repository is dirty. Please commit before preparing a release."
    exit 1
fi

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PACKAGEDIR=$SCRIPTDIR/translator-$MAJOR
mkdir -p $PACKAGEDIR/
#cp -r $SCRIPTDIR/translator/* $PACKAGEDIR/
cp -r $SCRIPTDIR/../../src/translate/* $PACKAGEDIR/
cp -r $SCRIPTDIR/../../LICENSE.md $PACKAGEDIR/
echo $MAJOR > $PACKAGEDIR/VERSION
python3 ../autodoc/generate-translate-docs.py --outdir $PACKAGEDIR/ --translator-package-documentation

ENVDIR=env-translate-$MAJOR
python3 -m venv $ENVDIR
source $ENVDIR/bin/activate
python3 -m pip install --upgrade build twine
cd $PACKAGEDIR
python3 -m build
twine check dist/*


## This needs to be a proper output but just some notes for me for the moment:
## activate the venv $ENVIR
## upload to TestPyPI with twine upload --repository testpypi dist/*
## BE SUPER CAREFUL: each version number can only be used once for an upload!!!
## (need to figure out what needs to be done so that everyone has the access rights.
