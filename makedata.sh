#!/bin/sh

# A shell script that uses gzip to compress public resources.

mkdir -p data/public
for v in $(ls public); do
	echo "$v"
	gzip --best -c "public/$v" > "data/$v.gz"
done
