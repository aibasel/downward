#!/bin/bash

## Assumptions:
##  * The repository containing this script is on the revision we want to release.
##  * The changelog is up to date and committed.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: ./prepare-release.sh 19.06.1"
    exit 1
fi

# Parse version parameter
VERSION=$1
if [[ ! "$VERSION" =~ ^[1-9][0-9]\.[0-9][0-9]\.[0-9]+$ ]]; then
    echo "Unrecognized version number '$VERSION'. Expected the format YY.MM.x (e.g. 19.06.0)."
    exit 1
fi
YEAR=${VERSION:0:2}
MONTH=${VERSION:3:2}
MAJOR="$YEAR.$MONTH"
MINOR=${VERSION##$MAJOR.}
BRANCH="release-$MAJOR"
TAG="release-$VERSION"

if [[ $MINOR = 0 ]]; then
    PRETTY_VERSION="$MAJOR"
else
    PRETTY_VERSION="$VERSION"
fi

# Set directories
SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
RELEASESDIR=$SCRIPTDIR
REPODIR="$(dirname $(dirname $SCRIPTDIR))"
pushd $REPODIR

# Verify that repository is clean
if [[ -n $(git status --porcelain) ]]; then
    echo "Repository is dirty. Please commit before preparing a release."
    exit 1
fi

function fill_template {
    TEMPLATE=$1
    PARAMETER=$2
    VALUE=$3
    sed -e "s/$PARAMETER/$VALUE/g" $SCRIPTDIR/templates/$TEMPLATE
}

function set_version {
    LOCALVERSION=$1
    fill_template _version.tpl VERSION "$LOCALVERSION" > $REPODIR/driver/version.py
    git add $REPODIR/driver/version.py
}

set -x

# Create the branch if it doesn't exist already.
if git rev-parse -q --verify "$BRANCH"; then
    if [[ $MINOR = 0 ]]; then
        echo "The version number '$VERSION' implies that this is the first release in branch '$BRANCH' but the branch already exists."
        exit 1
    fi
    if [[ "$(git rev-parse --abbrev-ref HEAD)" != "$BRANCH" ]]; then
        echo "It looks like we want to do a bugfix release, but we are not on the branch '$BRANCH'. Update to the branch head first."
        exit 1
    fi
else
    if [[ $MINOR != 0 ]]; then
        echo "The version number '$VERSION' implies a bugfix release but there is no branch '$BRANCH' yet."
        exit 1
    fi
    git checkout -b "$BRANCH"
fi

# Update version number.
set_version "$PRETTY_VERSION"

# Generate the different recipe files for Docker and Vagrant.
pushd $RELEASESDIR

mkdir -p $MAJOR
fill_template "_Dockerfile.tpl" "TAG" "$TAG" > $MAJOR/Dockerfile.$MAJOR
fill_template "_Vagrantfile.tpl" "TAG" "$TAG" > $MAJOR/Vagrantfile.$MAJOR
git add $MAJOR

mkdir -p latest
ln -fs ../$MAJOR/Dockerfile.$MAJOR latest/Dockerfile
ln -fs ../$MAJOR/Vagrantfile.$MAJOR latest/Vagrantfile
git add latest

popd

git commit -m "[$BRANCH] Release version $VERSION."

# Tag release.
git tag $TAG

# Back on the default branch, update version number.
if [[ $MINOR = 0 ]]; then
    git checkout main
    git merge --ff $BRANCH
    set_version "${MAJOR}+"
    git commit -m "[main] Update version number to ${MAJOR}+."
fi

# Create tarball. (ignored files are configured in .gitattributes)
ARCHIVE=fast-downward-$PRETTY_VERSION
git archive --prefix=$ARCHIVE/ -o $ARCHIVE.tar.gz $TAG

popd

cat << EOF
===============================================================================
Successfully prepared tag $TAG.
Please continue with the steps on
https://www.fast-downward.org/ForDevelopers/ReleaseWorkflow
EOF
