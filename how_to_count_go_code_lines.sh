#!/bin/bash

for f in $(find $1 -name "*.go"); do wc -l $f | awk '{print $1}'; done | awk '{s+=$1} END {print s}' 
