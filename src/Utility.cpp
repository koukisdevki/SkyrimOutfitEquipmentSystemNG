//
// Created by m on 8/22/2022.
//

#include "Utility.h"

#include "SKSE/SKSE.h"

#undef GetModuleFileName
#undef GetModuleHandle

std::string GetRuntimePath() {
    static char appPath[4096] = {0};

    if (appPath[0])
        return appPath;

    if (!SKSE::WinAPI::GetModuleFileName(SKSE::WinAPI::GetModuleHandle((const char*) nullptr), appPath, sizeof(appPath))) {
        SKSE::stl::report_and_fail("Failed to get runtime path");
    }

    return appPath;
}

std::string GetRuntimeName() {
    std::string appPath = GetRuntimePath();

    std::string::size_type slashOffset = appPath.rfind('\\');
    if (slashOffset == std::string::npos)
        return appPath;

    return appPath.substr(slashOffset + 1);
}

const std::string& GetRuntimeDirectory() {
    static std::string s_runtimeDirectory;

    if (s_runtimeDirectory.empty()) {
        std::string runtimePath = GetRuntimePath();

        // truncate at last slash
        std::string::size_type lastSlash = runtimePath.rfind('\\');
        if (lastSlash != std::string::npos)// if we don't find a slash something is VERY WRONG
        {
            s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);
        } else {
            LOG(critical, "no slash in runtime path? (%s)", runtimePath.c_str());
        }
    }

    return s_runtimeDirectory;
}

Settings::Settings() : reader(GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\SkyrimOutfitEquipmentSystemNG.ini") {
    if (reader.ParseError() != 0) {
        // Failed to load INI. We proceed without it.
        LOG(info, "Could not load INI file from {}. Continuing without it.", GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\SkyrimOutfitEquipmentSystemNG.ini");
        return;
    } else {
        LOG(info, "INI file was successfully loaded.");
    }
}

Settings::~Settings() {}

static Settings* settings;

INIReader* Settings::Instance() {
    if (!settings) settings = new Settings();
    return &settings->reader;
}

bool REUtilities::IsActorSleeping(RE::Actor* a_actor) {
    if (!a_actor) {
        LOG(info, "Actor is None");
        return false;
    }

    RE::ActorState* actorState = a_actor->AsActorState();

    RE::SIT_SLEEP_STATE sleepState = actorState->GetSitSleepState();

    return sleepState >= RE::SIT_SLEEP_STATE::kIsSleeping && sleepState <= RE::SIT_SLEEP_STATE::kWantToWake;
}

GameDayPart REUtilities::CurrentGameDayPart() {
    GameTime gameTime = CurrentGameHour();
    return (gameTime.hour >= 6 && gameTime.hour < 20) ? GameDayPart::Day : GameDayPart::Night;
}

GameTime REUtilities::CurrentGameHour() {
    GameTime result = {0, 0};

    auto calendar = RE::Calendar::GetSingleton();
    if (!calendar) {
        LOG(critical, "Failed to get Calendar singleton");
        throw std::runtime_error("Failed to get Calendar singleton");
    }

    // Get the hour as float (0.0-23.999...)
    float gameHourFloat = calendar->GetHour();

    // Extract integer hour and minute
    result.hour = static_cast<int>(gameHourFloat);

    // Fix: Explicitly cast result.hour to float before subtraction
    result.minute = static_cast<int>((gameHourFloat - static_cast<float>(result.hour)) * 60.0f);

    return result;
}

// Utility function for random number generation
int REUtilities::GetRandomInt(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(min, max);
    return distr(gen);
}

void REUtilities::ProcessArmorLeveledListEntry(const RE::LEVELED_OBJECT* entry, std::vector<RE::TESObjectARMO*>& outArmors, std::uint16_t playerLevel) {
    if (!entry || !entry->form) {
        return;
    }

    // Process based on quantity
    for (std::uint16_t i = 0; i < entry->count; i++) {
        // If it's an armor, add it directly
        if (entry->form->IsArmor()) {
            RE::TESObjectARMO* armor = entry->form->As<RE::TESObjectARMO>();
            if (armor) {
                outArmors.push_back(armor);
            }
        }
        // If it's another leveled list, process it recursively
        else if (entry->form->Is(RE::FormType::LeveledItem)) {
            ResolveArmorLeveledList(entry->form->As<RE::TESLevItem>(), outArmors, playerLevel);
        }
    }
}

// Helper function to resolve armors from a leveled list
void REUtilities::ResolveArmorLeveledList(RE::TESLevItem* levItem, std::vector<RE::TESObjectARMO*>& outArmors, std::uint16_t playerLevel) {
    if (!levItem) {
        return;
    }

    // Check chanceNone - chance that nothing is selected
    std::uint8_t chanceNone = levItem->GetChanceNone();
    if (chanceNone > 0) {
        // If roll is less than or equal to chanceNone, return nothing
        if (GetRandomInt(1, 100) <= chanceNone) {
            return;
        }
    }

    // Check if the "Use All" flag is set
    bool useAll = (levItem->llFlags & RE::TESLeveledList::Flag::kUseAll) != 0;

    // If "Use All" flag is set, process all valid entries
    if (useAll) {
        for (auto i = 0; i < levItem->entries.size(); i++)  {
            ProcessArmorLeveledListEntry(&levItem->entries[i], outArmors, playerLevel);
        }
    }
    // Otherwise, select one random entry
    else {
        int selectedIndex = GetRandomInt(0, static_cast<int>(levItem->entries.size()) - 1);
        ProcessArmorLeveledListEntry(&levItem->entries[selectedIndex], outArmors, playerLevel);
    }
}