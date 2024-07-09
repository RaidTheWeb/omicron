#! /bin/bash

# Very skeleton program designed only to facilitate easy usage of the runtime libraries.

echo "Running with Omicron Engine's runtime libraries..."

LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" $1 ${@:2}
