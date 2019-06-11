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

# Verify that Dockerfile exists
DOCKERFILE="$DOWNWARD_CONTAINER_REPO/$MAJOR/Dockerfile.$MAJOR"
if [ ! -f "$DOCKERFILE" ]; then
    echo "Could not find Dockerfile at '$DOCKERFILE'. Please run ./prepare-release.sh $MAJOR.0 first."
    exit 1
fi

set -x

# Take the generated Dockerfile, put it into a temporary empty directory,
# and build it. The directory can be removed afterwards as the docker
# image is stored in the local registry.
TEMPDIR=$(mktemp -d)
pushd $TEMPDIR
cp $DOCKERFILE Dockerfile
docker build -t aibasel/downward:$MAJOR .
docker tag aibasel/downward:$MAJOR aibasel/downward:latest
popd
rm -rf $TEMPDIR

docker login
docker push aibasel/downward:$MAJOR
docker push aibasel/downward:latest

docker rmi aibasel/downward:$MAJOR
docker image prune -f
