//
// Created by Koukibyou on 3/24/2025.
//

#include "Forms.h"
#include "OutfitSystemCacheService.h"

#include <google/protobuf/util/json_util.h>

OutfitSystemCacheService::OutfitSystemCacheService(const proto::OutfitSystemCache& data) {
    try {
        std::string protoData;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        auto status = google::protobuf::util::MessageToJsonString(data, &protoData, options);

        ActorVirtualInventoryStashes stashes;

        EXTRALOG(info, "Reading the following stored outfit system cache data:\n {}", protoData);

        for (const auto& actorStash : data.actor_virtual_inventory_stashes()) {
            // Lookup the actor
            std::uint64_t handle;
            const std::string& actorRefFormString = actorStash.actor_ref_form_string();

            RE::Actor* actor = skyrim_cast<RE::Actor*>(Forms::ParseFormString(actorRefFormString));

            if (!actor) continue;

            std::unordered_set<RE::TESObjectARMO*> armors;

            for (const auto& armorFormString : actorStash.armors_form_strings()) {
                RE::TESObjectARMO* armor = skyrim_cast<RE::TESObjectARMO*>(Forms::ParseFormString(armorFormString));

                if (!armor) continue;

                armors.insert(armor);
            }

            if (!armors.empty()) {
                stashes[actor] = armors;
            }

            actorVirtualInventoryStashes = stashes;
        }

        LOG(info, "Loaded {} stashes", stashes.size());
    }
    catch (const std::exception &e) {
        // print the exception
        LOG(info, "Exception initializing the armor override service, %s");
    }
}

proto::OutfitSystemCache OutfitSystemCacheService::save() {
    proto::OutfitSystemCache out;

    for (const auto& [actor, armors] : actorVirtualInventoryStashes) {
        // Create a new stash message pointer
        proto::ActorVirtualInventoryStash* stashOut = out.add_actor_virtual_inventory_stashes();

        // Set the fields on the created message
        stashOut->set_actor_ref_form_string(Forms::GetFormString(actor));

        // Add each armor formID to the repeated field
        for (const auto& armor : armors) {
            stashOut->add_armors_form_strings(Forms::GetFormString(armor));
        }
    }

    return out;
}

bool OutfitSystemCacheService::SetLoveSceneStateForActor(RE::Actor* actor, bool state) {
    //get armor service
    auto& armorService = ArmorAddonOverrideService::GetInstance();

    if (!armorService.actorOutfitAssignments.contains(actor)) return false;

    if (!actorStates.contains(actor)) actorStates[actor] = ActorStateCache();
    actorStates[actor].loveScene = state;

    return true;
}

std::optional<OutfitSystemCacheService::ActorStateCache> OutfitSystemCacheService::GetStateForActor(RE::Actor* actor) {
    auto& armorService = ArmorAddonOverrideService::GetInstance();

    if (!armorService.actorOutfitAssignments.contains(actor)) return std::nullopt;

    if (!actorStates.contains(actor)) return std::nullopt;

    return actorStates[actor];
}