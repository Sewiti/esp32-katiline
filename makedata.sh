#!/bin/sh

# A shell script that uses gzip to compress public resources.

process() {
	echo "$1"
	gzip --best -c "$1" > "data/$1.gz"
}

process public/index.html
process public/styles.css
process public/main.js
