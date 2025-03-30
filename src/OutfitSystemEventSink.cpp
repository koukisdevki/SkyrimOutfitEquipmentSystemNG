//
// Created by Koukibyou on 3/29/2025.
//

#include "OutfitSystemEventSink.h"

#include <Utility.h>

#include "ArmorAddonOverrideService.h"
#include "Forms.h"
#include "OutfitSystemCacheService.h"

enum EventType: std::uint16_t {
    Apply = 1,
    Middle = 2,
    Remove = 3,
};

bool DoesActorHaveActiveEffect(RE::Actor* actor, RE::EffectSetting* effect) {
    auto magicTarget = actor->AsMagicTarget();
    if (!magicTarget) return false;

    return magicTarget->HasMagicEffect(effect);
}

RE::BSEventNotifyControl CheckLoveSceneEffects(RE::TESObjectREFR* target, std::optional<RE::FormID> magicEvent, EventType effectType) {
    if (!target) {
        // LOG(info, "Actor is None");
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::Actor* actor = target->As<RE::Actor>();
    if (!actor) {
        // LOG(info, "Actor is None");
        return RE::BSEventNotifyControl::kContinue;
    }

    auto& armorService = ArmorAddonOverrideService::GetInstance();

    if (!armorService.actorOutfitAssignments.contains(actor)) return RE::BSEventNotifyControl::kContinue;

    static bool initCheck = false;
    static RE::TESForm* fgHeadEffectForm = nullptr;
    static RE::TESForm* fgLowerEffectForm = nullptr;

    if (fgHeadEffectForm == nullptr && !initCheck) {
        fgHeadEffectForm = Forms::ParseFormString(LoveSceneStringFormID::FlowerGirlLightHeadEffect);
    }

    if (fgLowerEffectForm == nullptr && !initCheck) {
        fgLowerEffectForm = Forms::ParseFormString(LoveSceneStringFormID::FlowerGirlLightLowerEffect);
    }

    initCheck = true;

    if (!fgHeadEffectForm || !fgLowerEffectForm) {
        // LOG(info, "All FG faction lights not found");
        return RE::BSEventNotifyControl::kContinue;
    }

    auto& systemCache = OutfitSystemCacheService::GetSingleton();
    bool result = false;

    if (magicEvent.has_value()) {
        if (magicEvent.value() == fgHeadEffectForm->formID || magicEvent.value() == fgLowerEffectForm->formID) result = false;
        systemCache.SetLoveSceneStateForActor(actor, result);

        return RE::BSEventNotifyControl::kContinue;
    }

    RE::EffectSetting* fgHeadLightEffect = skyrim_cast<RE::EffectSetting*>(fgHeadEffectForm);
    RE::EffectSetting* fgLowerLightEffect = skyrim_cast<RE::EffectSetting*>(fgLowerEffectForm);

    if (!fgHeadLightEffect || !fgLowerLightEffect) {
        // LOG(info, "Could not convert fg light effect forms to effect objects.");
        return RE::BSEventNotifyControl::kContinue;
    }

    LOG(info, "Checking if actor has FG effects");

    result = DoesActorHaveActiveEffect(actor, fgHeadLightEffect) || DoesActorHaveActiveEffect(actor, fgLowerLightEffect);

    if (result) {
        LOG(info, "Actor {} has FG effects, event type {}", actor->GetName(), effectType == Apply ? "Apply" : "Remove");
    }
    else {
        LOG(info, "Actor {} determined not to have FG effects after event type {}", actor->GetName(), effectType == Apply ? "Apply" : "Remove");
    }

    LOG(info, "Compared (formID {} OR formID {}) vs formID {}", fgHeadLightEffect->formID, fgLowerLightEffect->formID, magicEvent.value());

    // change love scene depending on the results
    systemCache.SetLoveSceneStateForActor(actor, result);

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OutfitSystemEventSink::ProcessEvent(const RE::TESMagicEffectApplyEvent* event,
                                                             RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {
    LOG(info, "Effect {} applied on {} by {}", event->magicEffect, event->target.get()->GetName(), event->caster.get()->GetName());

    CheckLoveSceneEffects(event->target.get(), event->magicEffect, Apply);

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OutfitSystemEventSink::ProcessEvent(const RE::TESQuestStartStopEvent* event,
                                                             RE::BSTEventSource<RE::TESQuestStartStopEvent>*) {
    LOG(info, "Starting/Stoping Quest {} started? {}", event->formID, event->started);

    if (event->started) return RE::BSEventNotifyControl::kContinue;

    auto& armorService = ArmorAddonOverrideService::GetInstance();

    for (const auto actor : armorService.actorOutfitAssignments | std::views::keys) {
        CheckLoveSceneEffects(actor, std::nullopt, event->started ? Apply : Middle);
    }

    return RE::BSEventNotifyControl::kContinue;
}