#!/bin/sh

for HTML in $(ls data/*.html); do
    gzip --force --keep --best "$HTML"
    # brotli --force --keep --best "$HTML"
done
