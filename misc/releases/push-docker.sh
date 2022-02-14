#!/bin/bash

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: ./push-docker.sh 19.06"
    exit 1
fi

MAJOR=$1
if [[ ! "$MAJOR" =~ ^[1-9][0-9]\.[0-9][0-9]$ ]]; then
    echo "Unrecognized version number '$MAJOR'. Expected the format YY.MM (e.g. 19.06)."
    exit 1
fi
SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
RELEASESDIR=$SCRIPTDIR
DOCKERFILE="$RELEASESDIR/$MAJOR/Dockerfile.$MAJOR"

SOPLEX_INSTALLER="$DOWNWARD_LP_INSTALLERS/soplex-3.1.1.tgz"

# Verify that the LP installer files exist
if [ ! -f "$SOPLEX_INSTALLER" ]; then
    echo "SoPlex 3.1.1 installation file not found at '$SOPLEX_INSTALLER'. Please set the environment variable DOWNWARD_LP_INSTALLERS to a path containing the SoPlex installer."
    exit 1
fi

function docker_build_and_tag {
    RECIPE_FILE=$1
    BUILD_TAG=$2
    if [ ! -f "$RECIPE_FILE" ]; then
        echo "Could not find Dockerfile at '$RECIPE_FILE'. Please run ./prepare-release.sh $MAJOR.0 first."
        exit 1
    fi
    # Take the generated Dockerfile, put it into a temporary directory with the
    # correct build context, and build it. The directory can be removed
    # afterwards as the docker image is stored in the local registry.
    TEMPDIR=$(mktemp -d)
    pushd $TEMPDIR
    cp $RECIPE_FILE Dockerfile
    cp $SOPLEX_INSTALLER .
    docker build -t $BUILD_TAG .
    popd
    rm -rf $TEMPDIR
}

set -x

docker_build_and_tag $DOCKERFILE "aibasel/downward:$MAJOR"
docker tag "aibasel/downward:$MAJOR" "aibasel/downward:latest"

docker login
docker push aibasel/downward:$MAJOR
docker push aibasel/downward:latest

docker rmi aibasel/downward:$MAJOR
docker image prune -f
