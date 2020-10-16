#!/bin/bash

for f in *.gml; do
    echo 'Creating $f'
    NAME=${f%%.*}
    NAME=${NAME#*/}
    if ./zoo-routing -z $f -t "networks/$NAME-topo.xml" -r "networks/$NAME-routing.xml" -q "networks/$NAME-queries.json"; then
        rm $f
    fi
done