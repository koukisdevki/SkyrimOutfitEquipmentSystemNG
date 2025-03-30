//
// Created by Koukibyou on 3/29/2025.
//

#pragma once

#include <RE/Skyrim.h>
#include "SKSE/Events.h"

class OutfitSystemEventSink :
    public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>,
    public RE::BSTEventSink<RE::TESQuestStartStopEvent>
{
    OutfitSystemEventSink() = default;
    OutfitSystemEventSink(const OutfitSystemEventSink&) = delete;
    OutfitSystemEventSink(OutfitSystemEventSink&&) = delete;
    OutfitSystemEventSink& operator=(const OutfitSystemEventSink&) = delete;
    OutfitSystemEventSink& operator=(OutfitSystemEventSink&&) = delete;

public:
    static OutfitSystemEventSink* GetSingleton() {
        static OutfitSystemEventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESMagicEffectApplyEvent* event,
                                          RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override;

    RE::BSEventNotifyControl ProcessEvent(const RE::TESQuestStartStopEvent* event,
                                          RE::BSTEventSource<RE::TESQuestStartStopEvent>*) override;
};
