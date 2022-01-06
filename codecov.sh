#!/usr/bin/env bash

CWD=$(pwd)
SCRIBE=build/bin/scribe

cd $1

# errors
## no file provided
echo "$SCRIBE"
$SCRIBE >/dev/null 2>&1
## file not found
echo "$SCRIBE no_file.sc"
$SCRIBE no_file.sc >/dev/null 2>&1
## compilation
echo "$SCRIBE error_examples/compile_error.sc"
$SCRIBE error_examples/compile_error.sc >/dev/null 2>&1

# version info
echo "$SCRIBE -v"
$SCRIBE -v >/dev/null 2>&1

# help
echo "$SCRIBE --help"
$SCRIBE --help >/dev/null 2>&1

# all examples
for x in examples/*; do echo "$SCRIBE $x"; $SCRIBE $x -s >/dev/null; rm $(basename ${x%.*}); done

cd $CWD
