#ifndef _ENGINE__ASSET_HPP
#define _ENGINE__ASSET_HPP

#include <engine/scripting/script.hpp>

struct asset_asset {
    union {
        struct dyn_script *script;
    };
};

#endif
