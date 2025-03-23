//
// Created by Koukibyou on 3/15/2025.
//

// AutoOutfitSwitchService.cpp
#include "AutoOutfitSwitchService.h"
#include "ArmorAddonOverrideService.h"

#include <Utility.h>

#include "OutfitSystem.h"

void AutoOutfitSwitchService::Initialize() {
    EnableMonitoring(true);
}

void AutoOutfitSwitchService::EnableMonitoring(bool enable) {
    // If already in the desired state, do nothing
    if (enable == isMonitoring) {
        return;
    }

    isMonitoring = enable;

    if (isMonitoring) {
        // Only start a new thread if one isn't already running
        if (!threadRunning.exchange(true)) {
            LOG(info, "Starting Autoswitch Thread");
            monitorThread = std::thread(&AutoOutfitSwitchService::MonitorThreadFunc, this);
            monitorThread.detach(); // Let it run independently - game may not shut down cleanly
        }
    }
}

void AutoOutfitSwitchService::RestartMonitoring() {
    // First disable monitoring to stop the thread
    EnableMonitoring(false);

    // Queue a task to restart monitoring after a brief delay
    // This happens asynchronously and won't block the main thread
    SKSE::GetTaskInterface()->AddTask([this]() {
        RE::DebugNotification("Starting skyrim outfit equipment system thread....");
        this->EnableMonitoring(true);
        StateReset();
    });
}

void AutoOutfitSwitchService::MonitorThreadFunc() {
    int accumulatedTimeMS = 0;

    while (isMonitoring) {
        constexpr int checkIntervalMS = 100;
        // Sleep for a shorter interval
        std::this_thread::sleep_for(std::chrono::milliseconds(checkIntervalMS));

        // Exit early if monitoring was disabled
        if (!isMonitoring) break;

        // Accumulate time until we reach the update interval
        accumulatedTimeMS += checkIntervalMS;
        if (accumulatedTimeMS >= updateIntervalMS) {
            // Queue a task to check for changes on the main thread
            SKSE::GetTaskInterface()->AddTask([this]() {
                RE::DebugNotification("Checking changes to location...");
                this->CheckForChanges();
            });

            // Reset accumulator
            accumulatedTimeMS = 0;
        }
    }

    LOG(info, "Closing Autoswitch Thread");

    threadRunning = false; // Mark thread as no longer running
}

void AutoOutfitSwitchService::StateReset() {
    LOG(info, "Auto Switch State Reset");

    // Clear current trackers
    actorStatusTrackers.clear();

    // Get the list of all actors
    auto& overrideService = ArmorAddonOverrideService::GetInstance();
    auto actors = overrideService.listActors();

    // Initialize tracker for each actor
    for (auto* actor : actors) {
        if (actor && actor->Is3DLoaded()) {
            ActorActionStatusTracker tracker;
            tracker.lastInCombatStatus = actor->IsInCombat();
            tracker.lastInWaterStatus = actor->IsInWater();
            tracker.lastSleepingStatus = REUtilities::IsActorSleeping(actor);
            tracker.lastSwimmingStatus = actor->AsActorState()->IsSwimming();
            tracker.lastOnMountStatus = actor->IsOnMount();

            actorStatusTrackers[actor] = tracker;
        } else {
            ActorActionStatusTracker tracker;
            tracker.lastInCombatStatus = false;
            tracker.lastInWaterStatus = false;
            tracker.lastSleepingStatus = false;
            tracker.lastSwimmingStatus = false;
            tracker.lastOnMountStatus = false;

            actorStatusTrackers[actor] = tracker;
        }
    }

    // Track environment info
    lastGameDayPart = REUtilities::CurrentGameDayPart();
    lastWeather = RE::Sky::GetSingleton()->currentWeather;
    auto player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        lastLocation = player->GetCurrentLocation();
    }
}

void AutoOutfitSwitchService::CheckForChanges() {
    if (!isMonitoring || isUpdating) {  // Don't process if already updating
        return;
    }

    // First check global environment changes
    auto player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        std::string message;

        // Check location changes
        const auto currentLocation = player->GetCurrentLocation();
        if (currentLocation != lastLocation) {
            std::string locationName = currentLocation && currentLocation->GetFullName() ?
                currentLocation->GetFullName() : "Unknown Location";
            UpdateOutfits("Location changed to " + locationName);
            // since on location changes new actors may have joined, do a full actor reset.
            StateReset();
            return; // Return to prevent multiple updates in the same check
        }

        // Check weather changes
        const auto currentWeather = RE::Sky::GetSingleton()->currentWeather;
        if (currentWeather != lastWeather && currentWeather) {
            UpdateOutfits("Weather changed to " + GetWeatherName(currentWeather));
            lastWeather = currentWeather;
            return; // Return to prevent multiple updates in the same check
        }

        // Check day part time changes
        const auto currentDayPart = REUtilities::CurrentGameDayPart();
        if (currentDayPart != lastGameDayPart) {
            std::string dayPartString = (currentDayPart == GameDayPart::Day) ? "Day" : "Night";
            UpdateOutfits("Day time changed to " + dayPartString);
            lastGameDayPart = currentDayPart;
            return;
        }
    }

    // Then check individual actor state changes
    for (auto& [actor, tracker] : actorStatusTrackers) {
        if (!actor) continue;

        if (!actor->Is3DLoaded()) {
            LOG(info, "The actor {} is not 3D loaded.", actor->GetDisplayFullName());
            continue;
        }

        std::string actorName = actor->GetName();
        if (actorName.empty()) {
            actorName = "Unnamed Actor";
        }
        std::string message;

        // Check combat status
        const bool currentlyInCombat = actor->IsInCombat();
        if (currentlyInCombat != tracker.lastInCombatStatus) {
            message = actorName + (currentlyInCombat ? " now in combat" : " no longer in combat");
            UpdateOutfits(message);
            tracker.lastInCombatStatus = currentlyInCombat;
            return;
        }

        // Check in water status
        const bool currentlyInWater = actor->IsInWater();
        if (currentlyInWater != tracker.lastInWaterStatus) {
            message = actorName + (currentlyInWater ? " now in water" : " no longer in water");
            UpdateOutfits(message);
            tracker.lastInWaterStatus = currentlyInWater;
            return;
        }

        // Check swimming status
        const bool currentSwimming = actor->AsActorState()->IsSwimming();
        if (currentSwimming != tracker.lastSwimmingStatus) {
            message = actorName + (currentSwimming ? " started swimming" : " stopped swimming");
            UpdateOutfits(message);
            tracker.lastSwimmingStatus = currentSwimming;
            return;
        }

        // Check sleeping status
        const bool currentlySleeping = REUtilities::IsActorSleeping(actor);
        if (currentlySleeping != tracker.lastSleepingStatus) {
            message = actorName + (currentlySleeping ? " started sleeping" : " stopped sleeping");
            UpdateOutfits(message);
            tracker.lastSleepingStatus = currentlySleeping;
            return;
        }

        // Check mounting status
        const bool currentlyOnMount = actor->IsOnMount();
        if (currentlyOnMount != tracker.lastOnMountStatus) {
            message = actorName + (currentlyOnMount ? " now on mount" : " no longer on mount");
            UpdateOutfits(message);
            tracker.lastOnMountStatus = currentlyOnMount;
            return;
        }
    }
}

void AutoOutfitSwitchService::UpdateOutfits(const std::string& reason) {
    // Use a separate flag to prevent duplicate updates while processing
    isUpdating = true;

    // Show notification
    RE::DebugNotification(("Outfit System: " + reason).c_str());

    // Update outfits using your existing native functions
    auto player = RE::PlayerCharacter::GetSingleton();

    if (player) {
        OutfitSystem::SetOutfitsUsingLocationRaw(player->GetCurrentLocation(), RE::Sky::GetSingleton()->currentWeather);
        OutfitSystem::RefreshArmorForAllConfiguredActorsRaw();
    }

    // Mark update as complete
    isUpdating = false;
}

std::string AutoOutfitSwitchService::GetWeatherName(RE::TESWeather* weather) {
    if (!weather) {
        return "Unknown";
    }

    // Get weather classification using the proper enumeration values
    using Flag = RE::TESWeather::WeatherDataFlag;

    const auto flags = weather->data.flags;
    if (flags.all(Flag::kPleasant)) return "Pleasant";
    if (flags.all(Flag::kCloudy)) return "Cloudy";
    if (flags.all(Flag::kRainy)) return "Rainy";
    if (flags.all(Flag::kSnow)) return "Snow";

    // Check for combination or other flags
    if (flags.any(Flag::kAuroraFollowsSun) || flags.any(Flag::kPermAurora)) {
        return "Aurora";
    }

    // Default case
    return "Weather " + std::to_string(reinterpret_cast<uintptr_t>(weather));
}