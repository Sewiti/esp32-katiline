#!/bin/sh

# A shell script that uses gzip to compress public resources.

mkdir -p data
cp -f public/* data/

gzip -9f        \
    data/*.html \
    data/*.css  \
    data/*.js   \
    data/*.ico  \
    data/*.svg
