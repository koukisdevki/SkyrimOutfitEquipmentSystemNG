//
// Created by m on 8/22/2022.
//

#include "Utility.h"

#include "Forms.h"
#include "SKSE/SKSE.h"

#undef GetModuleFileName
#undef GetModuleHandle

std::string GetRuntimePath() {
    static char appPath[4096] = {0};

    if (appPath[0])
        return appPath;

    if (!::GetModuleFileNameA(::GetModuleHandleA(nullptr), appPath, sizeof(appPath))) {
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


std::string ProtoUtils::readMessageAsJSON(const google::protobuf::Message& data) {
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    std::string dataAsJson;
    auto status = google::protobuf::json::MessageToJsonString(data, &dataAsJson, options);

    return dataAsJson;
}

std::optional<proto::UserInputJSON> GetUserTextInputJSONData() {
    std::string inputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\SkyrimOutfitEquipmentSystemNGTextInput.json";
    std::ifstream file(inputFile);
    if (!file) {
        LOG(critical, "Failed to open config for user text input json data");
        return std::nullopt;
    }
    std::stringstream input;
    input << file.rdbuf();
    if (!file.good()) {
        LOG(critical, "Failed to read user text input json data.");
        return std::nullopt;
    }

    proto::UserInputJSON data;

    auto status = google::protobuf::util::JsonStringToMessage(input.str(), &data);
    if (!status.ok()) {
        LOG(critical,"Failed to parse user text input json data. Invalid syntax.");
        return std::nullopt;
    }

    EXTRALOG(info, "Read the following data: ", ProtoUtils::readMessageAsJSON(data));

    return data;
}

std::string UserTextInputJSON::GetUserTextInputJSONOption(TextInputOption option) {
    std::optional<proto::UserInputJSON> dataRetrieveAttempt = GetUserTextInputJSONData();

    if (!dataRetrieveAttempt.has_value()) {
        LOG(critical, "Failed to retrieve JSON text input data");
        return "";
    }

    proto::UserInputJSON data = dataRetrieveAttempt.value();

    switch (option) {
        case ArmorFilterByName: return data.armor_filter_name();
        case AddToOutfitFormId: return data.add_to_outfit_by_form_id();
        default: return "";
    }
}

Settings::Settings() : reader(GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\SkyrimOutfitEquipmentSystemNG.ini") {
    if (reader.ParseError() != 0) {
        // Failed to load INI. We proceed without it.
        FORCELOG(info, "Could not load INI file from {}. Continuing without it.", GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\SkyrimOutfitEquipmentSystemNG.ini");
        return;
    }

    FORCELOG(info, "INI file was successfully loaded.");
}

Settings::~Settings() {}

static Settings* settings;

INIReader* Settings::Instance() {
    if (!settings) settings = new Settings();
    return &settings->reader;
}

bool Settings::LoggingEnabled() {
    static std::optional<bool> result;

    if (!result.has_value()) {
        result = Instance()->GetBoolean("Debug", "Logging", SettingsDefaults::Logging);
        EXTRALOG(info, "Logging set as {}", result.value());
    }

    return result.has_value() ? result.value() : SettingsDefaults::Logging;
}

bool Settings::ExtraLoggingEnabled(){
    static std::optional<bool> result;

    if (!result.has_value()) {
        result = LoggingEnabled() && Instance()->GetBoolean("Debug", "ExtraLogging", SettingsDefaults::ExtraLogging);
        EXTRALOG(info, "ExtraLogging set as {}", result.value());
    }

    return result.has_value() ? result.value() : SettingsDefaults::ExtraLogging;
}

int32_t Settings::OutfitPaginationCount(){
    static std::optional<int32_t> result;

    if (!result.has_value()) {
        result = Instance()->GetInteger("Menu", "OutfitPaginationCount", SettingsDefaults::OutfitPaginationCount);
        EXTRALOG(info, "OutfitPaginationCount set as {}", result.value());
    }

    return result.has_value() ? result.value() : SettingsDefaults::OutfitPaginationCount;
}

int32_t Settings::MenuPaginationCount() {
    static std::optional<int32_t> result;

    if (!result.has_value()) {
        result = Instance()->GetInteger("Menu", "MenuPaginationCount", SettingsDefaults::MenuPaginationCount);
        EXTRALOG(info, "MenuPaginationCount set as {}", result.value());
    }

    return result.has_value() ? result.value() : SettingsDefaults::MenuPaginationCount;
}
int32_t Settings::PollingMSInterval() {
    static std::optional<int32_t> result;

    if (!result.has_value()) {
        result = Instance()->GetInteger("Gameplay", "PollingMS", SettingsDefaults::PollingMS);
        EXTRALOG(info, "PollingMS set as {}", result.value());

        if (result < 500) {
            result = 500;
            EXTRALOG(info, "PollingMS cannot be lower than 500ms, setting to 500ms", result.value());
        }
    }

    return result.has_value() ? result.value() : SettingsDefaults::PollingMS;
}

bool Settings::AllowExternalEquipment() {
    static std::optional<bool> result;

    if (!result.has_value()) {
        result = LoggingEnabled() && Instance()->GetBoolean("Gameplay", "AllowExternalEquipment", SettingsDefaults::AllowExternalEquipment);
        EXTRALOG(info, "AllowExternalEquipment set as {}", result.value());
    }

    return result.has_value() ? result.value() : SettingsDefaults::AllowExternalEquipment;
}

void REUtilities::DebugNotification(const std::string& notification) {
    RE::DebugNotification(notification.c_str());
}

void REUtilities::ExtraDebugNotification(const std::string& notification) {
    if (Settings::ExtraLoggingEnabled()) {
        RE::DebugNotification(notification.c_str());
    }
}

std::unordered_set<RE::TESFaction*> REUtilities::GetActorFactions(RE::Actor* actor, bool forceRefresh) {
    // Use actor's form ID as a unique key
    static std::unordered_map<RE::Actor*, std::unordered_set<RE::TESFaction*>> cachedFactions;

    if (!actor) {
        return {};
    }

    // Check if we have cached results and not forcing a refresh
    if (!forceRefresh && cachedFactions.contains(actor)) {
        return cachedFactions[actor];
    }

    // Compute factions if not cached or forcing refresh
    std::unordered_set<RE::TESFaction*> factions;
    actor->VisitFactions([&](RE::TESFaction* faction, std::int8_t) {
        if (faction) {
            factions.insert(faction);
            return false;
        }
        return true;
    });

    // Cache the result
    cachedFactions[actor] = factions;

    return factions;
}

std::unordered_set<std::string> REUtilities::GetActorFactionEditorIDs(RE::Actor* actor, bool forceRefresh) {
    std::unordered_set<std::string> factionFormIDs;

    auto factions = GetActorFactions(actor, forceRefresh);

    for (auto faction: factions) {
        factionFormIDs.insert(get_editorID(faction));
    }

    return factionFormIDs;
}

bool REUtilities::IsActorInFlowerGirlScene(RE::Actor* actor) {
    if (!actor) {
        EXTRALOG(info, "Actor is None");
        return false;
    }

    static bool initCheck = false;
    static RE::TESForm* fgFactionForm;

    if (fgFactionForm == nullptr && !initCheck) {
        fgFactionForm = Forms::ParseFormString(LoveSceneStringFormID::FlowerGirlFaction);
        initCheck = true;
    }

    if (!fgFactionForm) {
        EXTRALOG(info, "No FG faction keywords found");
        return false;
    }

    RE::TESFaction* fgAnimationFaction = skyrim_cast<RE::TESFaction*>(fgFactionForm);

    if (!fgAnimationFaction) {
        EXTRALOG(info, "Could not convert fgFaction form to faction");
        return false;
    }

    EXTRALOG(info, "Checking if actor is in FG faction");

    bool result = actor->IsInFaction(fgAnimationFaction);

    if (result) {
        EXTRALOG(info, "Actor {} is in FG faction", actor->GetName());
    }
    else {
        EXTRALOG(info, "Actor {} is not in FG faction", actor->GetName());
    }

    return result;
}

bool REUtilities::IsFlowerGirlsInLoadOrder() {
    // faction check to see if its loaded
    auto fgFactionForm = Forms::ParseFormString(LoveSceneStringFormID::FlowerGirlFaction);
    return fgFactionForm != nullptr;
}

bool REUtilities::IsActorSleeping(RE::Actor* actor) {
    if (!actor) {
        EXTRALOG(trace, "Actor is None");
        return false;
    }

    RE::ActorState* actorState = actor->AsActorState();

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

std::vector<RE::TESObjectARMO*> REUtilities::OutfitToArmorList(RE::BGSOutfit* outfit){
    std::vector<RE::TESObjectARMO*> outfitArmors;

    // Iterate through outfit items and resolve them
    for (const auto& outfitItem : outfit->outfitItems) {
        // If the item is a leveled list, resolve its armors
        if (outfitItem->Is(RE::FormType::LeveledItem)) {
            // Cast to TESLevItem
            RE::TESLevItem* levItem = outfitItem->As<RE::TESLevItem>();
            if (levItem) {
                // Iterate through the leveled list entries
                for (const auto& outfitItem : outfit->outfitItems) {
                    // If the item is a leveled list, resolve its armors
                    if (outfitItem->Is(RE::FormType::LeveledItem)) {
                        RE::TESLevItem* levItem = outfitItem->As<RE::TESLevItem>();
                        if (levItem) {
                            // Get player level - you may need to adjust how you get this
                            std::uint16_t playerLevel = RE::PlayerCharacter::GetSingleton()->GetLevel();
                            REUtilities::ResolveArmorLeveledList(levItem, outfitArmors, playerLevel);
                        }
                    }
                    // If the item is an armor, add it directly
                    else if (outfitItem->IsArmor()) {
                        RE::TESObjectARMO* armor = outfitItem->As<RE::TESObjectARMO>();
                        if (armor) {
                            outfitArmors.push_back(armor);
                        }
                    }
                }
            }
        }
        // If the item is an armor, add it directly
        else if (outfitItem->IsArmor()) {
            RE::TESObjectARMO* armor = outfitItem->As<RE::TESObjectARMO>();
            if (armor) {
                outfitArmors.push_back(armor);
            }
        }
    }

    return outfitArmors;
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