#! /bin/bash

if [[ ! -z $DEBUG ]]; then
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" gdb ./bin/omicron
elif [[ ! -z $VALGRIND ]]; then
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" valgrind ./bin/omicron
else
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" ./bin/omicron
fi
