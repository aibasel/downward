#!/bin/bash

## Assumptions:
##  * The repository containing this script is on the revision we want to release.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: ./prepare-translator-package.sh 19.06"
    exit 1
fi
VERSION=$1

#if [[ ! "$VERSION" =~ ^[1-9][0-9]\.[0-9][0-9]$ ]]; then
#    echo "Unrecognized version number '$VERSION'. Expected the format YY.MM (e.g. 19.06)."
#    exit 1
#fi

# Verify that the repository is clean.
if [[ -n $(git status --porcelain) ]]; then
    echo "Repository is dirty. Please commit before preparing a release."
    exit 1
fi

# Prepare the files for the package.
SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PACKAGEDIR=$SCRIPTDIR/translator-$VERSION
mkdir $PACKAGEDIR/
#cp -r $SCRIPTDIR/translator/* $PACKAGEDIR/
cp -r $SCRIPTDIR/../../src/translate/* $PACKAGEDIR/
cp -r $SCRIPTDIR/../../LICENSE.md $PACKAGEDIR/
echo $VERSION > $PACKAGEDIR/VERSION
python3 $SCRIPTDIR/../autodoc/generate-translate-docs.py --outdir $PACKAGEDIR/ --translator-package-documentation
BRANCH=$(git rev-parse --abbrev-ref HEAD)
COMMIT=$(git rev-parse --short=12 HEAD)

cat >> $PACKAGEDIR/README.md <<EOF

---
*Built from Fast Downward branch \`$BRANCH\`, commit \`$COMMIT\`.*
EOF

# Build the package.
ENVDIR=$SCRIPTDIR/env-translate-$VERSION
python3 -m venv $ENVDIR
source $ENVDIR/bin/activate
python3 -m pip install --upgrade build twine
cd $PACKAGEDIR
python3 -m build
twine check dist/*
deactivate

echo ""
echo "##  Follow the following steps to upload the release to PyPI (requires credentials)".
echo "##  Please note that EVERY VERSION NUMBER CAN ONLY BE USED ONCE on PyPI (and Test-PyPI)."
echo "##  If you have doubts that everything will work fine, it's safer to first use an unused"
echo "##  fake version number like 0.0.3 for a test on Test-PyPI."
echo "##  "
echo "##  STEP 1: Activate environment and switch to package directory..."
echo "##  source $ENVDIR/bin/activate  # twine is installed in the environment"
echo "##  cd $PACKAGEDIR"
echo "##  "
echo "##  STEP 2: Test it on Test-PyPI..."
echo "##  twine upload --repository testpypi dist/*  # uploads the package to Test-PyPI"
echo "##  Try the package on Test-PyPI to verify that everything works as intended. You"
echo "##  can use script $SCRIPTDIR/test-latest-test-PyPI-translator.sh for this."
echo "##  "
echo "##  STEP 3: Actual upload to PyPI"
echo "##  twine upload dist/*  # uploads the package to PyPI"
echo "##  "
echo "##  Afterwards you can deactivate the virtual environment and delete directories"
echo "##  $PACKAGEDIR and"
echo "##  $ENVDIR."
