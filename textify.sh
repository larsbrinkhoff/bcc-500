#!/bin/sh

cd raw
for i in *; do
    j="../text/$i"
    ../textify < "$i" > "$j" || rm -f "$j"
done
