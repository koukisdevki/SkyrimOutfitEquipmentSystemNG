//
// Created by Koukibyou on 3/15/2025.
//

#pragma once

#include <Utility.h>

// Add to header:
#include <thread>
#include <atomic>
#include <chrono>

#include "RE/Skyrim.h"

// AutoOutfitSwitchService.cpp
#include "AutoOutfitSwitchService.h"

#include "OutfitSystemCacheService.h"


enum class WEAPON_STATE : std::uint32_t
{
    kSheathed = 0,
    kWantToDraw = 1,
    kDrawing = 2,
    kDrawn = 3,
    kWantToSheathe = 4,
    kSheathing = 5
};

struct ActorActionStatusTracker {
    RE::TESWeather* lastWeather = nullptr;
    RE::BGSLocation* lastLocation = nullptr;
    std::optional<GameDayPart> lastGameDayPart = std::nullopt;
    bool initialized = false;
    bool last3DLoadedStatus = false;
    bool lastInLoveSceneStatus = false;
    bool lastInCombatStatus = false;
    bool lastInWaterStatus = false;
    bool lastSleepingStatus = false;
    bool lastSwimmingStatus = false;
    bool lastOnMountStatus = false;
};

class AutoOutfitSwitchService {
public:
    static AutoOutfitSwitchService& GetSingleton() {
        static AutoOutfitSwitchService singleton;
        return singleton;
    }

    // Delete copy/move constructors and assignment operators
    AutoOutfitSwitchService(const AutoOutfitSwitchService&) = delete;
    AutoOutfitSwitchService(AutoOutfitSwitchService&&) = delete;
    AutoOutfitSwitchService& operator=(const AutoOutfitSwitchService&) = delete;
    AutoOutfitSwitchService& operator=(AutoOutfitSwitchService&&) = delete;

    void Initialize();
    void EnableMonitoring(bool enable);
    void RestartMonitoring();
    void CheckForChanges();
    void UpdateOutfits(const std::string& reason, int delayMS = 0);
    void StateReset();

private:
    AutoOutfitSwitchService() = default;
    ~AutoOutfitSwitchService() = default;

    bool isMonitoring = false;
    bool isUpdating = false;
    std::uint32_t updateIntervalMS = 3000; // 3 seconds
    std::thread monitorThread;
    std::atomic<bool> threadRunning{false};

    std::map<RE::Actor*, ActorActionStatusTracker> actorStatusTrackers;

    std::string GetWeatherName(RE::TESWeather* weather);
    void MonitorThreadFunc();
};