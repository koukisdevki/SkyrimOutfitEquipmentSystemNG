#include "ArmorAddonOverrideService.h"
#include "AutoOutfitSwitchService.h"
#include "OutfitSystemCacheService.h"

#pragma once

namespace RE {
    namespace BSScript {
        class IVirtualMachine;
    }
}// namespace RE

namespace OutfitSystem {
    bool RegisterPapyrus(RE::BSScript::IVirtualMachine* registry);
    void SetOutfitsUsingLocationRaw(RE::BGSLocation* location_skse,
                                RE::TESWeather* weather_skse);
    void RefreshArmorForAllConfiguredActorsRaw();

    struct EquipObject
    {
        static inline constexpr REL::RelocationID     relocation = RELOCATION_ID(37938, 38894);
        static inline std::size_t offset = REL::Relocate(0xe5, 0x170);

        static void thunk(RE::ActorEquipManager* manager, RE::Actor* actor, RE::TESBoundObject* object, RE::ExtraDataList* list)
        {
            if (actor && object && object->IsArmor()) {
                RE::TESObjectARMO* armor = static_cast<RE::TESObjectARMO*>(object);

                // LOG(info, "HOOK [EQUIP] {} equips {} armor.", actor->GetFormID(), armor->GetFormID());

                // For actors managed, only allow the equipment function to go through
                // afterwards process our changes if it is a tracked actor
                auto& armorAddonOverrideService = ArmorAddonOverrideService::GetInstance();

                if (armorAddonOverrideService.actorOutfitAssignments.contains(actor) && actor->Is3DLoaded()) {
                    bool isPlayerCharacter = (actor == RE::PlayerCharacter::GetSingleton());

                    // LOG(info, "Intercepted function call for actor");
                    auto& outfitAssignment = armorAddonOverrideService.actorOutfitAssignments.at(actor);

                    if (!isPlayerCharacter && !outfitAssignment.currentOutfitName.empty() && armorAddonOverrideService.outfits.contains(outfitAssignment.currentOutfitName)) {
                        auto currentOutfitArmors = armorAddonOverrideService.outfits.at(outfitAssignment.currentOutfitName).m_armors;

                        // When in an exception state, i.e love scene, let that system equip/unquip whatever it wants.
                        bool inExceptionState = false;

                        auto& cacheService = OutfitSystemCacheService::GetSingleton();
                        std::optional<OutfitSystemCacheService::ActorStateCache> actorStateCacheOpt = cacheService.GetStateForActor(actor);

                        // Exceptions
                        if (REUtilities::IsActorInLoveScene(actor) || (actorStateCacheOpt.has_value() && actorStateCacheOpt.value().loveScene)) inExceptionState = true;

                        // If the armor to be equipped is not part of the list, then don't equip anything.
                        if (!currentOutfitArmors.empty() && !currentOutfitArmors.contains(armor) && !inExceptionState) {
                            LOG(info, "Intercepted actor {}'s equip. Cannot equip {} because its not part of {}", actor->GetDisplayFullName(), armor->GetFormID(), outfitAssignment.currentOutfitName);
                            return;
                        }
                    }
                }
            }

            func(manager, actor, object, list);
        }

        static inline void post_hook()
        {
            LOG(info, "\t\t🪝Installed EquipObject hook.");
        }

        static inline REL::Relocation<decltype(thunk)> func;
    };

}