//
// Created by Koukibyou on 3/24/2025.
//

#pragma once

#include "cache.pb.h"

#include "RE/Skyrim.h"

#include "ArmorAddonoverrideService.h"

class OutfitSystemCacheService {
public:
    struct ActorStateCache {
        bool loveScene = false;
    };

    typedef std::map<RE::Actor*, std::unordered_set<RE::TESObjectARMO*>> ActorVirtualInventoryStashes;
    typedef std::map<RE::Actor*, ActorStateCache> ActorStates;

    static OutfitSystemCacheService& GetSingleton() {
        static OutfitSystemCacheService singleton;
        return singleton;
    }

    static constexpr std::uint32_t signature = 'OSCS';

    enum {
        kSaveVersionV1 = 1,
    };



    // a stash contains previously added armors, which gets reevaluated every armor switch
    ActorVirtualInventoryStashes actorVirtualInventoryStashes;
    ActorStates actorStates;

    OutfitSystemCacheService(){}
    OutfitSystemCacheService(const proto::OutfitSystemCache& data);// can throw load_error

    proto::OutfitSystemCache save();

    struct save_error: public std::runtime_error {
        explicit save_error(const std::string& what_arg) : runtime_error(what_arg){};
    };

    struct load_error: public std::runtime_error {
        explicit load_error(const std::string& what_arg) : runtime_error(what_arg){};
    };

    bool SetLoveSceneStateForActor(RE::Actor* actor, bool state);
    std::optional<ActorStateCache> GetStateForActor(RE::Actor* actor);
};