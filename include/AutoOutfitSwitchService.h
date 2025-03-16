//
// Created by Koukibyou on 3/15/2025.
//

#pragma once

#ifndef AUTOOUTFITSWITCHSERVICE_H
#define AUTOOUTFITSWITCHSERVICE_H

#endif //AUTOOUTFITSWITCHSERVICE_H

// Add to header:
#include <thread>
#include <atomic>
#include <chrono>

#include "RE/Skyrim.h"


enum class WEAPON_STATE : std::uint32_t
{
    kSheathed = 0,
    kWantToDraw = 1,
    kDrawing = 2,
    kDrawn = 3,
    kWantToSheathe = 4,
    kSheathing = 5
};

class AutoOutfitSwitchService {
public:
    static AutoOutfitSwitchService& GetInstance() {
        static AutoOutfitSwitchService instance;
        return instance;
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
    void UpdateOutfits(const std::string& reason);
    void Reset();

private:
    AutoOutfitSwitchService() = default;
    ~AutoOutfitSwitchService() = default;

    bool isMonitoring = false;
    bool isUpdating = false;
    std::uint32_t updateIntervalMS = 3000; // 3 seconds
    std::thread monitorThread;
    std::atomic<bool> threadRunning{false};

    RE::TESWeather* lastWeather = nullptr;
    RE::BGSLocation* lastLocation = nullptr;
    bool lastSwimmingStatus = false;
    bool lastInWaterStatus = false;

    std::string GetWeatherName(RE::TESWeather* weather);
    void MonitorThreadFunc();
};