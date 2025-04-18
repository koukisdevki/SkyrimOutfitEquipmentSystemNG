#include <Windows.h>
#include <google/protobuf/json/json.h>
#include <google/protobuf/util/json_util.h>

#include "ArmorAddonOverrideService.h"
#include "AutoOutfitSwitchService.h"
#include "Hooking.h"
#include "OutfitSystem.h"
#include "OutfitSystemCacheService.h"
#include "OutfitSystemEventSink.h"
#include "Utility.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

void WaitForDebugger(void) {
    while (!IsDebuggerPresent()) {
        Sleep(10);
    }

    Sleep(1000 * 2);
}

int ReportHook(int reportType, char* message, int* returnValue) {
    // Got an error
    util::report_and_fail(message);
}

namespace {
    void InitializeLog() {
        auto path = SKSE::log::log_directory();
        if (!path) {
            util::report_and_fail("Failed to find standard logging directory"sv);
        }

        *path /= fmt::format("{}.log"sv, Plugin::NAME);
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

        // Activate logging of everything until we load the log setting.
        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
        log->set_level(spdlog::level::trace);
        log->flush_on(spdlog::level::trace);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%H:%M:%S.%e] %g(%#): [%^%l%$] %v"s);

        // Load the actual log setting we should use.
        auto level = spdlog::level::info;
        bool deepLogEnabled = Settings::ExtraLoggingEnabled();
        if (deepLogEnabled) {
            LOG(info, "Extra logging enabled.");
            level = spdlog::level::trace;
        } else {
            LOG(info, "Extra logging disabled.");
        }

        // Set the actual log setting from hereon.
        spdlog::default_logger()->set_level(level);
        spdlog::default_logger()->flush_on(level);
    }
}

std::uint32_t g_pluginSerializationSignature = 'cOzt';

void Callback_Messaging_SKSE(SKSE::MessagingInterface::Message* message);
void Callback_Serialization_Save(SKSE::SerializationInterface* intfc);
void Callback_Serialization_Load(SKSE::SerializationInterface* intfc);

void Game_Full_Load_Initialize_Callback() {
    if (REUtilities::IsFlowerGirlsInLoadOrder()) {
        LOG(info, "Flowergirls detected, adding support.");

        auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
        auto* eventSink = OutfitSystemEventSink::GetSingleton();

        eventSourceHolder->AddEventSink<RE::TESMagicEffectApplyEvent>(eventSink);
        eventSourceHolder->AddEventSink<RE::TESQuestStartStopEvent>(eventSink);
    }

    // AAOS::load resets as well, but this is needed in case the save we're about to load doesn't have any AAOS
    // data.
    ArmorAddonOverrideService::GetInstance() = ArmorAddonOverrideService();

    // 'OSCS' fresh instance as well
    OutfitSystemCacheService::GetSingleton() = OutfitSystemCacheService();
}

void Callback_Messaging_SKSE(SKSE::MessagingInterface::Message* message) {
    if (message->type == SKSE::MessagingInterface::kPostLoad) {
        // Install hooks

        if (!Settings::AllowExternalEquipment()) {
            Hooking::install_hook<OutfitSystem::EquipObject>();
        } else {
            LOG(info, "Allowing external equipment.");
        }

    } else if (message->type == SKSE::MessagingInterface::kPostPostLoad) {
    } else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
    } else if (message->type == SKSE::MessagingInterface::kNewGame) {
        Game_Full_Load_Initialize_Callback();

        auto pc = RE::PlayerCharacter::GetSingleton();
        ArmorAddonOverrideService::GetInstance().addActor(pc);

        AutoOutfitSwitchService::GetSingleton().Initialize();
    } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
        Game_Full_Load_Initialize_Callback();
    }
    else if (message->type == SKSE::MessagingInterface::kPostLoadGame) {
        AutoOutfitSwitchService::GetSingleton().Initialize();
    }
}

void _assertWrite(bool result, const char* err);
void _assertRead(bool result, const char* err);

void Callback_Serialization_Save(SKSE::SerializationInterface* intfc) {
    LOG(info, "Writing savedata...");
    //
    if (intfc->OpenRecord(ArmorAddonOverrideService::signature, ArmorAddonOverrideService::kSaveVersionV1)) {
        try {
            auto& service = ArmorAddonOverrideService::GetInstance();
            const auto& data = service.save();
            const auto& data_ser = data.SerializeAsString();
            _assertWrite(intfc->WriteRecordData(data_ser.data(), static_cast<std::uint32_t>(data_ser.size())),
                         "Failed to write proto into SKSE record.");
        } catch (const ArmorAddonOverrideService::save_error& exception) {
            LOG(info, "Save FAILED for ArmorAddonOverrideService.");
            LOG(info, " - Exception string: %s", exception.what());
        }
    }
    else
        LOG(info, "Save FAILED for ArmorAddonOverrideService. Record didn't open.");

    if (intfc->OpenRecord(OutfitSystemCacheService::signature, OutfitSystemCacheService::kSaveVersionV1)) {
        try {
            auto& service = OutfitSystemCacheService::GetSingleton();
            const auto& data = service.save();
            const auto& data_ser = data.SerializeAsString();
            _assertWrite(intfc->WriteRecordData(data_ser.data(), static_cast<std::uint32_t>(data_ser.size())),
                         "Failed to write proto into SKSE record.");

            if (Settings::ExtraLoggingEnabled()) EXTRALOG(info, "saved outfit system cache {} to save.", ProtoUtils::readMessageAsJSON(data));
        } catch (const OutfitSystemCacheService::save_error& exception) {
            LOG(info, "Save FAILED for OutfitSystemCacheService.");
            LOG(info, " - Exception string: %s", exception.what());
        }
    }
    else
        LOG(info, "Save FAILED for ArmorAddonOverrideService. Record didn't open.");

    //
    LOG(info, "Saving done!");
}

void Callback_Serialization_Load(SKSE::SerializationInterface* intfc) {
    LOG(info, "Loading savedata...");
    //
    std::uint32_t type;  // This IS correct. A std::uint32_t and a four-character ASCII string have the same length (and
                         // can be read interchangeably, it seems).
    std::uint32_t version;
    std::uint32_t length;
    bool error = false;
    //
    while (!error && intfc->GetNextRecordInfo(type, version, length)) {
        switch (type) {
            case ArmorAddonOverrideService::signature:
                try {
                    auto& service = ArmorAddonOverrideService::GetInstance();
                    if (version >= ArmorAddonOverrideService::kSaveVersionV1) {
                        // Read data from protobuf.
                        std::vector<char> buf;
                        buf.insert(buf.begin(), length, 0);
                        _assertRead(intfc->ReadRecordData(buf.data(), length) == length, "Failed to load protobuf.");

                        // Parse data in protobuf.
                        proto::OutfitSystem data;
                        _assertRead(data.ParseFromArray(buf.data(), static_cast<int>(buf.size())),
                                    "Failed to parse protobuf.");

                        // Load data from protobuf struct.
                        service = ArmorAddonOverrideService(data, intfc);
                        LOG(info, "Succesfully loaded protobuf data for ArmorAddonOverrideService.");
                    } else {
                        LOG(err, "Legacy format not supported. Try upgrading first.");
                    }
                } catch (const ArmorAddonOverrideService::load_error& exception) {
                    LOG(info, "Load FAILED for ArmorAddonOverrideService.");
                    LOG(info, " - Exception string: %s", exception.what());
                }
                break;
            case OutfitSystemCacheService::signature:
                try {
                    auto& service = OutfitSystemCacheService::GetSingleton();
                    if (version >= OutfitSystemCacheService::kSaveVersionV1) {
                        // Read data from protobuf.
                        std::vector<char> buf;
                        buf.insert(buf.begin(), length, 0);
                        _assertRead(intfc->ReadRecordData(buf.data(), length) == length, "Failed to load protobuf.");

                        // Parse data in protobuf.
                        proto::OutfitSystemCache data;
                        _assertRead(data.ParseFromArray(buf.data(), static_cast<int>(buf.size())),
                                    "Failed to parse protobuf.");

                        // Load data from protobuf struct.
                        service = OutfitSystemCacheService(data);
                        LOG(info, "Succesfully loaded protobuf data for OutfitSystemCacheService.");
                        if (Settings::ExtraLoggingEnabled()) EXTRALOG(info, " Data: {}.", ProtoUtils::readMessageAsJSON(data));
                    } else {
                        LOG(err, "Legacy format not supported. Try upgrading first.");
                    }
                } catch (const OutfitSystemCacheService::load_error& exception) {
                    LOG(info, "Load FAILED for ArmorAddonOverrideService.");
                    LOG(info, " - Exception string: %s", exception.what());
                }
                break;
            default:
                LOG(info, "Loading: Unhandled type %c%c%c%c", (char)(type >> 0x18), (char)(type >> 0x10),
                    (char)(type >> 0x8), (char)type);
                error = true;
                break;
        }
    }
    //
    LOG(info, "Loading done!");
    // reset state of autoswitch service
    AutoOutfitSwitchService::GetSingleton().StateReset();
}

SKSEPluginLoad(const LoadInterface* a_skse) {
    SKSE::Init(a_skse);

    InitializeLog();
    LOG(info, "Load: {} v{}", Plugin::NAME, Plugin::VERSION.string());

    auto gameVersion = REL::Relocate("SE", "AE", "VR");
    LOG(info, "Game type: {}", gameVersion);

    auto version = REL::Module::get().version();
    LOG(info, "Game version: {}", version.string());

    LOG(info, "Is VR Mode enabled?? {}", REL::Module::IsVR());

#ifdef _DEBUG
    // Intercept Visual C++ exceptions, but only if we're developing.
    _CrtSetReportHook(ReportHook);
#endif

    LOG(info, "Loaded the outfit system SKSE.");

    // Check some interface versions
    if (SKSE::GetMessagingInterface()->Version() < SKSE::MessagingInterface::kVersion) {
        LOG(critical, "Messaging interface too old.");
        return false;
    }

    if (SKSE::GetSerializationInterface()->Version() < SKSE::SerializationInterface::kVersion) {
        LOG(critical, "Serialization interface too old.");
        return false;
    }

    // Messaging Callback
    LOG(info, "Registering messaging callback");
    SKSE::GetMessagingInterface()->RegisterListener(Callback_Messaging_SKSE);

    // Serialization Callbacks
    LOG(info, "Registering serialization callback");
    SKSE::GetSerializationInterface()->SetUniqueID(g_pluginSerializationSignature);
    SKSE::GetSerializationInterface()->SetSaveCallback(Callback_Serialization_Save);
    SKSE::GetSerializationInterface()->SetLoadCallback(Callback_Serialization_Load);

    // Papyrus Registrations
    LOG(info, "Registering papyrus");
    SKSE::GetPapyrusInterface()->Register(OutfitSystem::RegisterPapyrus);

    return true;
}
