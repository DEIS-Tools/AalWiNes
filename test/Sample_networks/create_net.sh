#!/bin/bash

for f in $(ls gml/*.gml)
do 
    echo "Creating $f"
    NAME=${f%%.*}
    if ../cmake-build-debug/bin/zoo-routing -z $f -t "../$NAME-topo.xml" -r "../$NAME-routing.xml" -q "../$NAME-queries.json"; then
        rm $f
    fi
    echo "Done $f"
done