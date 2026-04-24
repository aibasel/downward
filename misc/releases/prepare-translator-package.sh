#!/bin/bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: ./prepare-translator-package.sh 19.06"
    exit 1
fi
MAJOR=$1

if [[ ! "$MAJOR" =~ ^[1-9][0-9]\.[0-9][0-9]$ ]]; then
    echo "Unrecognized version number '$MAJOR'. Expected the format YY.MM (e.g. 19.06)."
    exit 1
fi

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
PACKAGEDIR=$SCRIPTDIR/translator-$MAJOR
mkdir -p $PACKAGEDIR/downward
cp -r $SCRIPTDIR/translator/* $PACKAGEDIR/
cp -r $SCRIPTDIR/../../downward/translate $PACKAGEDIR/downward/
cp -r $SCRIPTDIR/../../LICENSE.md $PACKAGEDIR/
echo $MAJOR > $PACKAGEDIR/VERSION

ENVDIR=env-translate-$MAJOR
python3 -m venv $ENVDIR
source $ENVDIR/bin/activate
python3 -m pip install --upgrade build
cd $PACKAGEDIR
python3 -m build
