#! /bin/bash

if [[ -z $GCC ]]; then
    echo "\$GCC is not defined."
    exit
fi

if [[ -z $LIBS ]]; then
    echo "\$LIBS is not defined."
    exit
fi

if [[ -z $HEADERS ]]; then
    echo "\$HEADERS is not defined."
    exit
fi

$GCC -fPIC -g -std=gnu11 -I$HEADERS \
    -I$LIBS/cglm/include \
    -I$LIBS/bgfx/include \
    -I$LIBS/bx/include \
    -I$LIBS/KTX-Software/include \
    -I$LIBS/meshoptimizer/src \
    -L$LIBS/meshoptimizer/build -o $2 -shared $1
