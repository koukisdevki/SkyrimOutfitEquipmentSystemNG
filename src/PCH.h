#ifndef SKYRIMOUTFITEQUIPMENTSYSTEMNG_SOS_PCH_H
#define SKYRIMOUTFITEQUIPMENTSYSTEMNG_SOS_PCH_H

#include "version.h"

#pragma warning(push)
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include "INIReader.h"
#include "SKSE/Impl/PCH.h"
#include "Utility.h"

#ifdef NDEBUG
#include <spdlog/sinks/basic_file_sink.h>
#else
#include <spdlog/sinks/basic_file_sink.h>
#endif
#pragma warning(pop)

using namespace std::literals;

#define FORCELOG(a_type, ...) \
    spdlog::log(spdlog::source_loc(__FILE__, __LINE__, __FUNCTION__), spdlog::level::a_type, __VA_ARGS__)

#define LOG(a_type, ...) \
    if (Settings::LoggingEnabled()) spdlog::log(spdlog::source_loc(__FILE__, __LINE__, __FUNCTION__), spdlog::level::a_type, __VA_ARGS__)

#define EXTRALOG(a_type, ...) \
    if (Settings::ExtraLoggingEnabled()) spdlog::log(spdlog::source_loc(__FILE__, __LINE__, __FUNCTION__), spdlog::level::a_type, __VA_ARGS__)

namespace util {
    using SKSE::stl::report_and_fail;
}

#define DllExport __declspec(dllexport)

namespace Plugin {
    using namespace std::literals;

    inline constexpr REL::Version VERSION{SKYRIMOUTFITEQUIPMENTSYSTEMNG_VERSION_MAJOR, SKYRIMOUTFITEQUIPMENTSYSTEMNG_VERSION_MINOR,
                                          SKYRIMOUTFITEQUIPMENTSYSTEMNG_VERSION_PATCH};
    inline constexpr auto NAME = "SkyrimOutfitEquipmentSystemNG"sv;
}  // namespace Plugin

class LogExit {
public:
    std::string_view m_string;
    LogExit(std::string_view name) : m_string(name) {
        LOG(trace, "Enter {}", m_string);
    };
    ~LogExit() {
        LOG(trace, "Exit {}", m_string);
    };
};

#endif  // SKYRIMOUTFITEQUIPMENTSYSTEMNG_SOS_PCH_H
