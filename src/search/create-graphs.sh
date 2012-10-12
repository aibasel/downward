#! /bin/bash

set -e

for FILE in `ls *.dot`; do
    echo $FILE
    dot -Tpng $FILE -o $FILE.png
done
eog *.png
