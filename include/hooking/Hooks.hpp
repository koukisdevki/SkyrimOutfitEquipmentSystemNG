//
// Created by m on 9/11/2022.
//

#ifndef SKYRIMOUTFITSYSTEMNG_SRC_HOOKING_HOOKS_AE_HPP
#define SKYRIMOUTFITSYSTEMNG_SRC_HOOKING_HOOKS_AE_HPP

namespace SKSE {
    class Trampoline;
}

namespace Hooking {
    enum HookID { 
        DontVanillaSkinPlayerHookID,
        TESObjectARMOApplyArmorAddonHookID, 
        ShimWornFlagsHookID,
        InventoryChangesGetWornMaskHookID,
        CustomSkinPlayerHookID,
        InventoryChangesExecuteVisitorOnWornHookID,
        FixEquipConflictCheckHookID,
        BGSBipedObjectFormTestBodyPartByIndexHookID,
        GetSkillToLevelHookID,
        SkillMutationFunctionHookID,
        BipedAnimVisitGetWornArmorTypeVisitorHookID,
        BipedAnimGetActorRaceBodyArmorPtrPtrHookID,
    };

    extern SKSE::Trampoline* g_localTrampoline;
    extern SKSE::Trampoline* g_branchTrampoline;

    inline REL::RelocationID GetHookRelocationID(HookID id) {
        switch (id) {
            case HookID::DontVanillaSkinPlayerHookID:
                // Name is: void TESObjectARMO::InitWornArmor(Actor* a_actor, BSTSmartPointer<BipedAnim>* a_biped)
                return RELOCATION_ID(24232, 24736);

            case HookID::TESObjectARMOApplyArmorAddonHookID:
                // void TESObjectARMO::AddToBiped(RE::TESObjectARMO* a_armor,RE::TESRace* a_race, BSTSmartPointer<BipedAnim>* a_biped, bool isFemale)
                return RELOCATION_ID(17392, 17792);

            case HookID::ShimWornFlagsHookID:
                // Not found for VR
                // AE: TESNPC::sub_*, close to TESNPC::UnClone3D_* (24722)
                return RELOCATION_ID(24220, 24724);

            case HookID::InventoryChangesGetWornMaskHookID:
                // std::uint32_t InventoryChanges::GetWornMask()
                return RELOCATION_ID(15806, 16044);

            case HookID::CustomSkinPlayerHookID:
                // Not found for VR
                // AE: TESNPC::sub_*, close to TESNPC::UnClone3D_* (24722)
                return RELOCATION_ID(24231, 24725);

            case HookID::InventoryChangesExecuteVisitorOnWornHookID:
                return RELOCATION_ID(15856, 16096);

            case HookID::FixEquipConflictCheckHookID:
                // Not found for VR
                // AE: AE: Actor::AddWornItem_
                return RELOCATION_ID(36979, 38004);

            case HookID::BGSBipedObjectFormTestBodyPartByIndexHookID:
                return RELOCATION_ID(14026, 14119);

            case HookID::GetSkillToLevelHookID:
                // this is temp, AE doesn't exist and its something else we're using
                // Not found for VR
                // AE: Character::sub_*
                return RELOCATION_ID(37589, 38627);

            case HookID::SkillMutationFunctionHookID:
                // this is temp, SE doesn't exist and its something else we're using
                // Not found for VR
                // AE: Character::sub_*
                return RELOCATION_ID(37589, 38627);

            case HookID::BipedAnimVisitGetWornArmorTypeVisitorHookID:
                // this is temp, AE doesn't exist and its something else we're using
                // Not found for VR
                // AE: BipedAnim::GetBodyObject_*
                return RELOCATION_ID(37688, 15696);

            case HookID::BipedAnimGetActorRaceBodyArmorPtrPtrHookID:
                // this is temp, SE doesn't exist and its something else we're using
                // Not found for VR
                // AE: BipedAnim::GetBodyObject_*
                return RELOCATION_ID(37688, 15696);

            default:
                return RELOCATION_ID(24232, 24736);
                //throw std::runtime_error("Unknown hook ID encountered in GetHookRelocationID");
        }
    }
}// namespace Hooking

namespace HookingPREAE {
    void ApplyPlayerSkinningHooks();
}

namespace HookingAE {
    void ApplyPlayerSkinningHooks();
}

#endif//SKYRIMOUTFITSYSTEMNG_SRC_HOOKING_HOOKS_AE_HPP
