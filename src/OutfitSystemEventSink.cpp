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

bool IsFormIDFlowerGirlsQuest(RE::FormID formId) {
    static bool initCheck = false;
    static std::string logString = "";
    static std::vector questFormIdStrings = {
        LoveSceneStringFormID::FlowerGirlSceneThread1,
        LoveSceneStringFormID::FlowerGirlSceneThread2,
        LoveSceneStringFormID::FlowerGirlSceneThread3,
        LoveSceneStringFormID::FlowerGirlSceneThread4,
        LoveSceneStringFormID::FlowerGirlSceneThread5,
    };

    static std::unordered_set<RE::FormID> fgQuestSceneFormIDs;

    if (initCheck && fgQuestSceneFormIDs.empty()) {
        for (const auto& questFormIdString: questFormIdStrings) {
            auto fgQuest = Forms::ParseFormString(questFormIdString);
            if (fgQuest && fgQuest->formID) {
                fgQuestSceneFormIDs.insert(fgQuest->formID);
                logString += "-"+std::to_string(fgQuest->formID)+"-";
            }
        }
    }

    initCheck = true;

    EXTRALOG(info, "Checking if formID {} is an fg quest within {}", formId, logString);

    return fgQuestSceneFormIDs.contains(formId);
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

    if (magicEvent.has_value()) {
        bool result = false;
        EXTRALOG(info, "Checking if actor has FG effects");

        if (magicEvent.value() == fgHeadEffectForm->formID || magicEvent.value() == fgLowerEffectForm->formID) result = true;

        if (result) {
            EXTRALOG(info, "Actor {} has FG effects, event type {}", actor->GetName(), effectType == Apply ? "Apply" : "Remove");
        }
        else {
            EXTRALOG(info, "Actor {} determined not to have FG effects after event type {}", actor->GetName(), effectType == Apply ? "Apply" : "Remove");
        }

        EXTRALOG(info, "Compared (formID {} OR formID {}) vs formID {}", fgHeadEffectForm->formID, fgLowerEffectForm->formID, magicEvent.value());

        systemCache.SetLoveSceneStateForActor(actor, result);

        return RE::BSEventNotifyControl::kContinue;
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OutfitSystemEventSink::ProcessEvent(const RE::TESMagicEffectApplyEvent* event,
                                                             RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) {

    if (event->target != event->caster) return RE::BSEventNotifyControl::kContinue;

    EXTRALOG(info, "Effect {} applied on {} by self", event->magicEffect, event->target.get()->GetName());

    CheckLoveSceneEffects(event->target.get(), event->magicEffect, Apply);

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl OutfitSystemEventSink::ProcessEvent(const RE::TESQuestStartStopEvent* event,
                                                             RE::BSTEventSource<RE::TESQuestStartStopEvent>*) {
    if (event->started == true) return RE::BSEventNotifyControl::kContinue;
    if (!IsFormIDFlowerGirlsQuest(event->formID)) return RE::BSEventNotifyControl::kContinue;

    EXTRALOG(info, "Starting/Stoping Quest {} started? {}", event->formID, event->started);

    auto& armorService = ArmorAddonOverrideService::GetInstance();
    auto& systemCache = OutfitSystemCacheService::GetSingleton();

    for (const auto actor : armorService.actorOutfitAssignments | std::views::keys) {
        systemCache.SetLoveSceneStateForActor(actor, REUtilities::IsActorInFlowerGirlScene(actor));
    }

    return RE::BSEventNotifyControl::kContinue;
}