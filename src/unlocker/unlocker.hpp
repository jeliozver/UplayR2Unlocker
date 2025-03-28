#pragma once

#include "koalabox/koalabox.hpp"

namespace unlocker {
    using namespace koalabox;

    typedef uint32_t ProductID;
    typedef uint32_t ChunkID;

    struct Config {
        bool logging = false;
        String lang = "default";
        bool auto_fetch = true;
        Set<ProductID> dlcs;
        Set<ProductID> items;
        Set<ProductID> blacklist;
        Set<ChunkID> chunks;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Config, logging, lang, auto_fetch, dlcs, items, blacklist, chunks)
    };

    extern Config config;

    extern HMODULE original_library;

    extern ProductID global_app_id;

    extern bool is_hook_mode;

    void init(const HMODULE& loader_library);

    void post_init();

    void shutdown();

}
