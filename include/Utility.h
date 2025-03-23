#pragma once

#include <string>

std::string GetRuntimeName();

const std::string& GetRuntimeDirectory();

class Settings {
public:
    Settings();
    ~Settings();
    INIReader reader;
    static INIReader* Instance();
};

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
    bool IsActorSleeping(RE::Actor* actor);
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