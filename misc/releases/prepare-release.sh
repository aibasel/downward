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
REPODIR="$(dirname $(dirname $SCRIPTDIR))"
pushd $REPODIR

# Verify that repository is clean
if [[ $(hg st) ]]; then
    echo "Repository is dirty. Please commit before preparing a release."
    exit 1
fi

# Verify that DOWNWARD_CONTAINER_REPO exists
if [ ! -d "$DOWNWARD_CONTAINER_REPO/.git" ]; then
    echo "Please set the envronment variable DOWNWARD_CONTAINER_REPO to the path of a local clone of https://github.com/aibasel/downward."
    exit 1
fi

function fill_template {
    TEMPLATE=$1
    PARAMETER=$2
    VALUE=$3
    sed -e "s/$PARAMETER/$VALUE/g" $SCRIPTDIR/templates/$TEMPLATE
}

function set_and_commit_version {
    LOCALVERSION=$1
    fill_template _version.tpl VERSION "$LOCALVERSION" > $REPODIR/driver/version.py
    hg commit -m "Update version number to $LOCALVERSION."
}

function create_recipe_and_link_latest {
    CONTAINERTYPE=$1
    PARAMETER=$2
    VALUE=$3
    fill_template "_$CONTAINERTYPE.tpl" "$PARAMETER" "$VALUE" > $MAJOR/$CONTAINERTYPE.$MAJOR
    ln -fs $MAJOR/$CONTAINERTYPE.$MAJOR latest/$CONTAINERTYPE
}

set -x

# Create the branch if it doesn't exist already.
if [[ $(hg branches | grep "^$BRANCH ") ]]; then
    if [[ $MINOR = 0 ]]; then
        echo "The version number '$VERSION' implies that this is the first release in branch '$BRANCH' but the branch already exists."
        exit 1
    fi
    if [[ "$(hg branch)" != "$BRANCH" ]]; then
        echo "It looks like we want to do a bugfix release, but we are not on the branch '$BRANCH'. Update to the branch head first."
        exit 1
    fi
else
    if [[ $MINOR != 0 ]]; then
        echo "The version number '$VERSION' implies a bugfix release but there is no branch '$BRANCH' yet."
        exit 1
    fi
    hg branch "$BRANCH"
    hg commit -m "Create branch $BRANCH."
fi

# Update version number.
set_and_commit_version "$PRETTY_VERSION"

# Tag release.
hg tag $TAG -m "Create tag $TAG."

# Back on the default branch, update version number.
if [[ $MINOR = 0 ]]; then
    hg update default
    set_and_commit_version "${MAJOR}+"
fi

# Create tarball.
hg archive -r $TAG -X .hg_archival.txt -X .hgignore \
    -X .hgtags -X .uncrustify.cfg -X bitbucket-pipelines.yml \
    -X experiments/ -X misc/ --type tgz \
    fast-downward-$PRETTY_VERSION.tar.gz

# Generate the different recipe files for Docker, Singularity and Vagrant.
pushd $DOWNWARD_CONTAINER_REPO

mkdir -p $MAJOR
fill_template "_Dockerfile.tpl" "TAG" "$TAG" > $MAJOR/Dockerfile.$MAJOR
fill_template "_Singularity.tpl" "MAJOR" "$MAJOR" > $MAJOR/Singularity.$MAJOR
fill_template "_Vagrantfile.tpl" "TAG" "$TAG" > $MAJOR/Vagrantfile.$MAJOR
git add $MAJOR

mkdir -p latest
ln -fs ../$MAJOR/Dockerfile.$MAJOR latest/Dockerfile
ln -fs ../$MAJOR/Singularity.$MAJOR latest/Singularity
ln -fs ../$MAJOR/Vagrantfile.$MAJOR latest/Vagrantfile
git add latest

git commit -m "Add recipe files for release $VERSION."
popd

popd

cat << EOF
Successfully prepared tag $TAG.
Please take the following steps to verify the release:
  * Check that fast-downward-$PRETTY_VERSION.tar.gz contains the correct files
  * Check that the branches and tags were created as intended
  * Check that $DOWNWARD_CONTAINER_REPO has a commit with the correct
    container recipe files.

Once you are satisfied with everything, execute the following commands
to publish the build:

cd $REPODIR
hg push
misc/release/push-docker.sh $MAJOR
git -C $DOWNWARD_CONTAINER_REPO push
EOF
