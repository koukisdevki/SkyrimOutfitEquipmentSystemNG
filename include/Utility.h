#pragma once

#include <google/protobuf/util/json_util.h>
#include "input.pb.h"

#include <string>

std::string GetRuntimeName();

const std::string& GetRuntimeDirectory();

namespace SettingsDefaults {
    static constexpr bool Logging = false;
    static constexpr bool ExtraLogging = false;
    static constexpr int32_t OutfitPaginationCount = 20;
    static constexpr int32_t MenuPaginationCount = 1000;
    static constexpr bool AllowExternalEquipment = false;
}

namespace UserTextInputJSON {
    enum TextInputOption {
        AddToOutfitFormId,
        ArmorFilterByName
    };

    std::string GetUserTextInputJSONOption(TextInputOption option);
};

class Settings {
public:
    Settings();
    ~Settings();
    INIReader reader;
    static INIReader* Instance();

    static bool LoggingEnabled();
    static bool ExtraLoggingEnabled();
    static int32_t OutfitPaginationCount();
    static int32_t MenuPaginationCount();
    static bool AllowExternalEquipment();
};

namespace ProtoUtils {
    std::string readMessageAsJSON(const google::protobuf::Message& data);
};

namespace LoveSceneStringFormID {
    const static std::string FlowerGirlFaction = "0x5BEF2C|FlowerGirls SE.esm";
    const static std::string FlowerGirlSceneThread1 = "0x5BEF2F|FlowerGirls SE.esm";
    const static std::string FlowerGirlSceneThread2 = "0x5C41DD|FlowerGirls SE.esm";
    const static std::string FlowerGirlSceneThread3 = "0x5F1C9B|FlowerGirls SE.esm";
    const static std::string FlowerGirlSceneThread4 = "0x5F1C9C|FlowerGirls SE.esm";
    const static std::string FlowerGirlSceneThread5 = "0x5F1C9D|FlowerGirls SE.esm";
    const static std::string FlowerGirlLightHeadEffect = "0x54A808|FlowerGirls SE.esm";
    const static std::string FlowerGirlLightLowerEffect = "0x54A80A|FlowerGirls SE.esm";
}

enum class GameDayPart : uint8_t {
    Day = 0,
    Night = 1,
};

struct GameTime {
    int hour;    // 0-23
    int minute;  // 0-59

    // Equality operator
    bool operator==(const GameTime& other) const {
        return hour == other.hour && minute == other.minute;
    }
};

namespace REUtilities {
    void DebugNotification(const std::string&);
    void ExtraDebugNotification(const std::string&);
    bool IsActorSleeping(RE::Actor* actor);
    std::unordered_set<RE::TESFaction*> GetActorFactions(RE::Actor* actor, bool forceRefresh = false);
    std::unordered_set<std::string> GetActorFactionEditorIDs(RE::Actor* actor, bool forceRefresh = false);
    bool IsFlowerGirlsInLoadOrder();
    bool IsActorInFlowerGirlScene(RE::Actor* actor);
    GameTime CurrentGameHour();
    GameDayPart CurrentGameDayPart();
    int GetRandomInt(int min, int max);
    std::vector<RE::TESObjectARMO*> OutfitToArmorList(RE::BGSOutfit* outfit);
    void ProcessArmorLeveledListEntry(const RE::LEVELED_OBJECT* entry, std::vector<RE::TESObjectARMO*>& outArmors, std::uint16_t playerLevel);
    void ResolveArmorLeveledList(RE::TESLevItem* levItem, std::vector<RE::TESObjectARMO*>& outArmors, std::uint16_t playerLevel);

    using _GetFormEditorID = const char* (*)(std::uint32_t);

    inline std::string get_editorID(const RE::TESForm* a_form)
    {
        switch (a_form->GetFormType()) {
            case RE::FormType::Keyword:
            case RE::FormType::LocationRefType:
            case RE::FormType::Action:
            case RE::FormType::MenuIcon:
            case RE::FormType::Global:
            case RE::FormType::HeadPart:
            case RE::FormType::Race:
            case RE::FormType::Sound:
            case RE::FormType::Script:
            case RE::FormType::Navigation:
            case RE::FormType::Cell:
            case RE::FormType::WorldSpace:
            case RE::FormType::Land:
            case RE::FormType::NavMesh:
            case RE::FormType::Dialogue:
            case RE::FormType::Quest:
            case RE::FormType::Idle:
            case RE::FormType::AnimatedObject:
            case RE::FormType::ImageAdapter:
            case RE::FormType::VoiceType:
            case RE::FormType::Ragdoll:
            case RE::FormType::DefaultObject:
            case RE::FormType::MusicType:
            case RE::FormType::StoryManagerBranchNode:
            case RE::FormType::StoryManagerQuestNode:
            case RE::FormType::StoryManagerEventNode:
            case RE::FormType::SoundRecord:
                return a_form->GetFormEditorID();
            default:
            {
                static auto tweaks = GetModuleHandle(L"po3_Tweaks");
                static auto func = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
                if (func) {
                    return func(a_form->formID);
                }
                return {};
            }
        }
    }
}