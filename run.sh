#! /bin/bash

if [[ ! -z $DEBUG ]]; then
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" gdb ./bin/omicron
elif [[ ! -z $VALGRIND ]]; then
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" valgrind --track-origins=yes ./bin/omicron
else
    LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(realpath rt/)" ./bin/omicron
fi
