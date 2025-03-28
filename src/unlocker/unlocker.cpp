#include "unlocker.hpp"
#include "upc/upc.hpp"

#include "koalabox/config_parser/config_parser.hpp"
#include "koalabox/loader/loader.hpp"
#include "koalabox/util/util.hpp"
#include "koalabox/win_util/win_util.hpp"

#include <build_config.h>

#define DETOUR(FUNC) \
    hook::detour(original_library, #FUNC, reinterpret_cast<FunctionPointer>(FUNC));

namespace unlocker {
    Config config = {}; // NOLINT(cert-err58-cpp)

    ProductID global_app_id = 0;

    HMODULE original_library = nullptr;

    bool is_hook_mode = false;

    void init(const HMODULE& self_module) {
        try {
            DisableThreadLibraryCalls(self_module);

            const auto self_directory = loader::get_module_dir(self_module);

            config = config_parser::parse<Config>(self_directory / PROJECT_NAME".jsonc", true);

            if (config.logging) {
                logger = file_logger::create(self_directory / PROJECT_NAME".log");
            }

            logger->info("🐨 {} 🔓 v{}", PROJECT_NAME, PROJECT_VERSION);

            const auto module_path = win_util::get_module_file_name(self_module);

            const auto is_not_original_dll = hook::is_hook_mode(self_module, ORIGINAL_DLL);
            const auto is_not_legacy_dll = hook::is_hook_mode(self_module, LEGACY_ORIGINAL_DLL);

            is_hook_mode = is_not_original_dll and is_not_legacy_dll;

            if (is_hook_mode) {
                logger->info("🪝 Detected hook mode");

                dll_monitor::init(STORE_DLL, [](const HMODULE& store_library) {
                    hook::init();

                    original_library = store_library;

                    DETOUR(UPC_Init)
                    DETOUR(UPC_InstallLanguageGet)
                    DETOUR(UPC_ProductListFree)
                    DETOUR(UPC_ProductListGet)
                    DETOUR(UPC_ApplicationIdGet)
                    DETOUR(UPC_InstallChunkListGet)
                    DETOUR(UPC_AchievementListGet)
                    DETOUR(UPC_AchievementListFree)

                    logger->info("Hooking complete");

                    dll_monitor::shutdown();
                });
            } else {
                logger->info("🔀 Detected proxy mode");

                original_library = loader::load_original_library(self_directory, ORIGINAL_DLL);
            }

            logger->info("🚀 Initialization complete");
        } catch (const Exception& ex) {
            util::panic(fmt::format("Initialization error: {}", ex.what()));
        }
    }

    void shutdown() {
        try {
            if (not is_hook_mode) {
                win_util::free_library(original_library);
            }

            logger->info("💀 Shutdown complete");
        } catch (const Exception& ex) {
            logger->error("Shutdown error: {}", ex.what());
        }
    }

}

