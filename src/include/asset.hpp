#ifndef _ASSET_HPP
#define _ASSET_HPP

#include <scripting/script.hpp>

struct asset_asset {
    union {
        struct dyn_script *script;
    };
};

#endif
