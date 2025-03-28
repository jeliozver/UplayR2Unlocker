#include "hook.hpp"
#include "koalabox/loader/loader.hpp"

#include "3rd_party/polyhook2.hpp"

namespace koalabox::hook {
    using namespace koalabox;

    class PolyhookLogger : public PLH::Logger {
        bool print_info;
    public:
        explicit PolyhookLogger(bool print_info = false) : print_info(print_info) {};

        void log(String msg, PLH::ErrorLevel level) override {
            if (level == PLH::ErrorLevel::INFO && print_info) {
                koalabox::logger->debug("[Polyhook] {}", msg);
            } else if (level == PLH::ErrorLevel::WARN) {
                koalabox::logger->warn("[Polyhook] {}", msg);
            } else if (level == PLH::ErrorLevel::SEV) {
                koalabox::logger->error("[Polyhook] {}", msg);
            }
        }
    };

#ifdef _WIN64
    typedef PLH::x64Detour Detour;
#else
    typedef PLH::x86Detour Detour;
#endif

    Map <String, FunctionPointer> address_book; // NOLINT(cert-err58-cpp)

    Vector<PLH::IHook*> hooks; // NOLINT(cert-err58-cpp)

    void detour_or_throw(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function,
        const PLH::x64Detour::detour_scheme_t scheme
    ) {
        logger->debug("Hooking '{}' via Detour", function_name);

        static PLH::CapstoneDisassembler disassembler(util::is_x64() ? PLH::Mode::x64 : PLH::Mode::x86);

        const auto address = reinterpret_cast<FunctionPointer>(
            win_util::get_proc_address(module, function_name.c_str())
        );

        uint64_t trampoline = 0;

        const auto detour = new Detour(address, callback_function, &trampoline, disassembler);

#ifdef _WIN64
        detour->setDetourScheme(scheme);
#endif
        if (detour->hook()) {
            address_book[function_name] = reinterpret_cast<FunctionPointer>(trampoline);

            hooks.push_back(detour);
        } else {
            throw util::exception("Failed to hook function: {}", function_name);
        }
    }

    void detour(
        const HMODULE& module,
        const String& function_name,
        FunctionPointer callback_function,
        const PLH::x64Detour::detour_scheme_t scheme
    ) {
        try {
            detour_or_throw(module, function_name, callback_function, scheme);
        } catch (const std::exception& ex) {
            util::panic("Failed to hook function {} via Detour: {}", function_name, ex.what());
        }
    }

    void eat_hook_or_throw(const HMODULE& module, const String& function_name, FunctionPointer callback_function) {
        logger->debug("Hooking '{}' via EAT", function_name);

        uint64_t orig_function_address = 0;
        const auto eat_hook = new PLH::EatHook(
            function_name,
            module,
            reinterpret_cast<FunctionPointer>(callback_function),
            &orig_function_address
        );

        if (eat_hook->hook()) {
            address_book[function_name] = reinterpret_cast<FunctionPointer>(orig_function_address);

            hooks.push_back(eat_hook);
        } else {
            delete eat_hook;

            throw util::exception("Failed to hook function: '{}'", function_name);
        }
    }

    FunctionPointer get_original_function(bool is_hook_mode, const HMODULE& library, const String& function_name) {
        if (is_hook_mode) {
            if (not hook::address_book.contains(function_name)) {
                util::panic("Address book does not contain function: {}", function_name);
            }

            return hook::address_book[function_name];
        } else {
            return reinterpret_cast<FunctionPointer>(
                win_util::get_proc_address(library, function_name.c_str())
            );
        }
    }

    void init(bool print_info) {
        logger->debug("Hooking initialization");

        // Initialize polyhook logger
        auto polyhook_logger = std::make_shared<PolyhookLogger>(print_info);
        PLH::Log::registerLogger(polyhook_logger);
    }

    bool is_hook_mode(const HMODULE& self_module, const String& orig_library_name) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = Path(module_path).filename().string();

        return not util::strings_are_equal(self_name, orig_library_name + ".dll");
    }
}
