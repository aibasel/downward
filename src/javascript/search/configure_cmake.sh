#!/bin/sh
cd "$(dirname "$0")/../../"
if [ $# -eq 0 ]; then
    echo "No arguments provided"
    echo "Run either with "
    echo "./configure_cmake.sh emscripten"
    echo "or "
    echo "./configure_cmake.sh native"
    exit 1
fi

if [ "$1" = "emscripten" ];
then 
	echo 'setting cmake settings for building with emscripten' 
    # in case of switch we need to execute twice, as cmake notices changed variables and executes itself again but fails as EMSCRIPTEN variable is not passed
	emcmake cmake -DEMSCRIPTEN=True -DCMAKE_CXX_COMPILER=$(which em++) .
    emcmake cmake -DEMSCRIPTEN=True -DCMAKE_CXX_COMPILER=$(which em++) .
elif [ "$1" = "native" ];
then
	echo "setting cmake settings for native build"
    # in case of switch we need to execute twice, as cmake notices changed variables and executes itself again but fails as EMSCRIPTEN variable is not passed
	cmake -DEMSCRIPTEN=False -DCMAKE_CXX_COMPILER=$(which g++) .
    cmake -DEMSCRIPTEN=False -DCMAKE_CXX_COMPILER=$(which g++) .
else 
    echo "argument not recognized"
    echo "Run either with "
    echo "./configure_cmake.sh emscripten"
    echo "or "
    echo "./configure_cmake.sh native"
    exit 1
fi