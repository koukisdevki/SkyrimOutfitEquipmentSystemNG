//
// Created by Koukibyou on 3/15/2025.
//

// AutoOutfitSwitchService.cpp
#include "AutoOutfitSwitchService.h"

#include <Utility.h>

#include "OutfitSystem.h"

void AutoOutfitSwitchService::Initialize() {
    Reset();
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

void AutoOutfitSwitchService::Reset() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        lastLocation = player->GetCurrentLocation();
        lastWeather = RE::Sky::GetSingleton()->currentWeather;
        lastInWaterStatus = REUtilities::IsActorInWater(player);
        lastSwimmingStatus = player->AsActorState()->IsSwimming();
    }
}

void AutoOutfitSwitchService::CheckForChanges() {
    if (!isMonitoring || isUpdating) {  // Don't process if already updating
        return;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        return;
    }

    std::string message;

    // Check location changes
    const auto currentLocation = player->GetCurrentLocation();
    if (currentLocation != lastLocation) {
        std::string locationName = currentLocation && currentLocation->GetFullName() ?
            currentLocation->GetFullName() : "Unknown Location";
        UpdateOutfits("Location changed to " + locationName);
        lastLocation = currentLocation;
        return; // Return to prevent multiple updates in the same check
    }

    // Check weather changes
    const auto currentWeather = RE::Sky::GetSingleton()->currentWeather;
    if (currentWeather != lastWeather && currentWeather) {
        UpdateOutfits("Weather changed to " + GetWeatherName(currentWeather));
        lastWeather = currentWeather;
        return; // Return to prevent multiple updates in the same check
    }

    // Check in water status
    const bool currentlyInWater = REUtilities::IsActorInWater(player);
    if (currentlyInWater != lastInWaterStatus) {
        message = currentlyInWater ? "Now in water" : "No longer in water";
        UpdateOutfits(message);
        lastInWaterStatus = currentlyInWater;
        return;
    }

    // Check swimming status
    const bool currentSwimming = player->AsActorState()->IsSwimming();
    if (currentSwimming != lastSwimmingStatus) {
        message = currentSwimming ? "Started swimming" : "Stopped swimming";
        UpdateOutfits(message);
        lastSwimmingStatus = currentSwimming;
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