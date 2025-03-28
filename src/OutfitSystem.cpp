#include "OutfitSystem.h"

#include <excpt.h>

#include <algorithm>

#include "OutfitSystemCacheService.h"
#include "Utility.h"
#include "cobb/strings.h"
#include "cobb/utf8naturalsort.h"
#include "cobb/utf8string.h"
#include "google/protobuf/util/json_util.h"

#define ERROR_AND_RETURN_EXPR_IF(condition, message, valueExpr, registry, stackId)               \
    if (condition) {                                                                             \
        registry->TraceStack(message, stackId, RE::BSScript::IVirtualMachine::Severity::kError); \
        return (valueExpr);                                                                      \
    }

#define ERROR_AND_RETURN_IF(condition, message, registry, stackId)                               \
    if (condition) {                                                                             \
        registry->TraceStack(message, stackId, RE::BSScript::IVirtualMachine::Severity::kError); \
        return;                                                                                  \
    }

namespace OutfitSystem {
    std::int32_t GetOutfitNameMaxLength(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*) {
        LogExit exitPrint("GetOutfitNameMaxLength"sv);
        return ArmorAddonOverrideService::ce_outfitNameMaxLength;
    }
    std::vector<RE::TESObjectARMO*> GetCarriedArmor(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*,
        RE::Actor* target) {
        LogExit exitPrint("GetCarriedArmor"sv);
        std::vector<RE::TESObjectARMO*> result;

        if (target == nullptr) {
            registry->TraceStack("Cannot retrieve data for a None RE::Actor.",
                                 stackId,
                                 RE::BSScript::IVirtualMachine::Severity::kError);
            return result;
        }

        // Use GetInventory() which returns a safely iterable container
        auto inventory = target->GetInventory();
        for (const auto& [item, data] : inventory) {
            if (item && item->Is(RE::FormType::Armor)) {
                auto armor = item->As<RE::TESObjectARMO>();
                if (armor && data.first > 0) {  // Check if the item count is > 0
                    result.push_back(armor);
                }
            }
        }

        return result;
    }

    std::vector<RE::TESObjectARMO*> GetWornItems(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*,
        RE::Actor* target) {
        LogExit exitPrint("GetWornItems"sv);
        std::vector<RE::TESObjectARMO*> result;

        if (target == nullptr) {
            registry->TraceStack("Cannot retrieve data for a None RE::Actor.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return result;
        }

        // Get inventory using GetInventory() which returns a safely iterable container
        auto inventory = target->GetInventory();
        for (const auto& [item, data] : inventory) {
            if (item && item->Is(RE::FormType::Armor)) {
                auto armor = item->As<RE::TESObjectARMO>();
                if (armor && data.second && data.second->IsWorn()) {
                    result.push_back(armor);
                }
            }
        }

        return result;
    }

    void RefreshArmorForActor(RE::Actor* target) {
        if (!target || !target->Is3DLoaded()) {
            LOG(info, "Actor {} not loaded", target->GetName());
            return;
        }

        auto& cacheService = OutfitSystemCacheService::GetSingleton();
        auto& svc = ArmorAddonOverrideService::GetInstance();
        auto& outfit = svc.currentOutfit(target);

        // Compute what should be displayed based on outfit settings
        unordered_set<RE::TESObjectARMO*> displayItems = outfit.m_armors;

        // Get the ActorEquipManager for equipment operations
        auto equipManager = RE::ActorEquipManager::GetSingleton();
        if (!equipManager) {
            LOG(critical,"Failed to get ActorEquipManager singleton");
            return;
        }

        // Get currently equipped items
        std::unordered_set<RE::TESObjectARMO*> equippedArmors;
        std::unordered_set<RE::TESObjectARMO*> outfitArmorsInInventory;

        bool isPlayerCharacter = target == RE::PlayerCharacter::GetSingleton();
        bool forceEquip = !isPlayerCharacter;  // true for NPCs, false for player
        InventoryManagementMode actorManagementMode = isPlayerCharacter ? svc.playerInventoryManagementMode : svc.npcInventoryManagementMode;

        // if no outfit, and the target is not the player, then equip default outfit
        if (outfit == g_noOutfit && target != RE::PlayerCharacter::GetSingleton() && !svc.listActors().contains(target)) {
            LOG(critical,"Actor {} has no outfit and not part of the list, attempting to set default outfit.", target->GetName());

            // Get the default outfit for this NPC
            auto defaultOutfit = target->GetActorBase()->defaultOutfit;

            // If there's no default outfit either, just return without doing anything
            if (!defaultOutfit) {
                LOG(critical,"Actor {} has no default outfit", target->GetName());
                return;
            }

            auto outfitArmors = REUtilities::OutfitToArmorList(defaultOutfit);
            displayItems = unordered_set<RE::TESObjectARMO*>();
            for (const auto& outfitArmor : outfitArmors) {
                displayItems.insert(outfitArmor);
            }
        }

        //If outfit exists, unequip armors that are not part of the outfit
        if (!displayItems.empty()) {
            auto inv = target->GetInventory();

            for (const auto& [item, data] : inv) {
                if (item && item->Is(RE::FormType::Armor)) {
                    auto armor = item->As<RE::TESObjectARMO>();
                    if (armor && data.second && data.second->IsWorn()) {
                        equippedArmors.insert(armor);
                    }

                    // if the current armor is part of the outfit, mark it as an inventory item
                    if (displayItems.contains(armor)) outfitArmorsInInventory.insert(armor);
                }
            }

            for (auto equippedArmor : equippedArmors) {
                // if the currently equipped outfit is not part of the outfit, remove it
                if (!displayItems.contains(equippedArmor)) {
                    auto* equipSlot = equippedArmor->GetEquipSlot();
                    equipManager->UnequipObject(target, equippedArmor, nullptr, 1, equipSlot, false, forceEquip, false, true);
                }
            }
        }

        // Equip items that should be displayed
        for (auto armor : displayItems) {
            // This armor should be equipped if not currently equipped
            if (!equippedArmors.contains(armor)) {
                // in immersive mode, only equip it if its part of the actor's inventory
                if (actorManagementMode == InventoryManagementMode::Immersive && !outfitArmorsInInventory.contains(armor)) continue;

                // if not part of the user's inventory items, and we're in automatic mode then add the items to actor's inventory, and add it as a stashed item.
                if (!outfitArmorsInInventory.contains(armor) && actorManagementMode == InventoryManagementMode::Automatic) {
                    target->AddObjectToContainer(armor, nullptr, 1, nullptr);

                    if (!cacheService.actorVirtualInventoryStashes.contains(target))
                        cacheService.actorVirtualInventoryStashes[target] = std::unordered_set<RE::TESObjectARMO*>();

                    cacheService.actorVirtualInventoryStashes[target].insert(armor);
                    LOG(info,"Added {} to {}'s inventory", armor->GetName(), target->GetName());
                }

                // Get the appropriate slot for this armor
                auto* equipSlot = armor->GetEquipSlot();

                // Use ActorEquipManager to equip
                equipManager->EquipObject(target, armor, nullptr, 1, equipSlot, false, forceEquip, false, true);
                LOG(info,"Equipped {} on {}", armor->GetName(), target->GetName());
            }
        }

        // In automatic mode, we remove all the previously added armors from the actor's inventory that are not part of the current outfit
        if (actorManagementMode == InventoryManagementMode::Automatic) {
            if (cacheService.actorVirtualInventoryStashes.contains(target)) {
                for (auto armor: cacheService.actorVirtualInventoryStashes[target]) {
                    if (!displayItems.contains(armor)) {
                        target->RemoveItem(armor, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
                        cacheService.actorVirtualInventoryStashes[target].erase(armor);
                    }
                }
            }
        }

        LOG(info,"Updated outfit for actor {}, ID: {}", target->GetName(), target->GetFormID());
        LOG(info,"Armors equipped: {}", displayItems.size());
    }

    void RefreshArmorFor(RE::BSScript::IVirtualMachine* registry,
                         std::uint32_t stackId,
                         RE::StaticFunctionTag*,
                         RE::Actor* target) {
        LogExit exitPrint("RefreshArmorFor"sv);
        ERROR_AND_RETURN_IF(target == nullptr, "Cannot refresh armor on a None RE::Actor.", registry, stackId);
        RefreshArmorForActor(target);
    }

    void RefreshArmorForAllConfiguredActorsRaw() {
        LogExit exitPrint("RefreshArmorForAllConfiguredActors"sv);

        try{
            auto& service = ArmorAddonOverrideService::GetInstance();
            auto actors = service.listActors();

            for (auto& actor : actors) {
                RefreshArmorForActor(actor);
                LOG(info,"Updated outfit for actor {}, ID: {}", actor->GetName(), actor->GetFormID());
            }
        }
        catch (const std::exception& e) {
            LOG(critical,"Failed to refresh armor for configued actors");
        }
    };

    void RefreshArmorForAllConfiguredActors(RE::BSScript::IVirtualMachine* registry,
                                            std::uint32_t stackId,
                                            RE::StaticFunctionTag*) {
        RefreshArmorForAllConfiguredActorsRaw();
    }

    std::vector<RE::Actor*> ActorsNearPC(RE::BSScript::IVirtualMachine* registry,
                                     std::uint32_t stackId,
                                     RE::StaticFunctionTag*) {
        LogExit exitPrint("ActorsNearPC"sv);
        std::vector<RE::Actor*> result;

        auto& svc = ArmorAddonOverrideService::GetInstance();
        auto pc = RE::PlayerCharacter::GetSingleton();

        // Get the process lists - this manages all active actors
        auto processLists = RE::ProcessLists::GetSingleton();
        ERROR_AND_RETURN_EXPR_IF(processLists == nullptr, "Could not get ProcessLists Singleton.", result, registry, stackId);

        // Track which actors we've already added to avoid duplicates
        std::unordered_set<RE::Actor*> addedActors;

        // do current cell NPCs
        auto pcCell = pc->GetParentCell();

        auto isValidPlayableActor = [&](RE::Actor* actor) -> bool {
            return actor && actor != pc && !addedActors.contains(actor) &&
                   actor->Is3DLoaded() && actor->GetRace() &&
                   actor->GetRace()->data.flags.all(RE::RACE_DATA::Flag::kPlayable);
        };

        if (pcCell) {
            for (const auto& ref : pcCell->GetRuntimeData().references) {
                RE::TESObjectREFR* objectRefPtr = ref.get();
                auto actor = skyrim_cast<RE::Actor*>(objectRefPtr);
                if (isValidPlayableActor(actor)) addedActors.insert(actor);
            }
        }

        // Helper function to process an actor handle array
        auto processHandles = [&](const RE::BSTArray<RE::ActorHandle>& handles) {
            for (const auto& handle : handles) {
                if (auto actor = handle.get().get()) {
                    // Skip the player character and actors we've already added
                    if (isValidPlayableActor(actor)) {
                        addedActors.insert(actor);
                    }
                }
            }
        };

        // Process all four handle arrays
        processHandles(processLists->highActorHandles);      // Fully active, closest to player
        processHandles(processLists->middleHighActorHandles); // Moderately active
        processHandles(processLists->middleLowActorHandles); // Less active
        // processHandles(processLists->lowActorHandles);       // not active, but still loaded. Makes process very slow.

        // add pc if they're not tracked
        if (!svc.actorOutfitAssignments.contains(pc)) result.push_back(pc);

        // results are only those currently not tracked
        for (auto& addedActor : addedActors) {
            if (!svc.actorOutfitAssignments.contains(addedActor)) {
                result.push_back(addedActor);
            }
        }

        return result;
    }

    //
    namespace ArmorFormSearchUtils {
        static struct {
            std::vector<std::string> names;
            std::vector<RE::TESObjectARMO*> armors;
            //
            void setup(std::string nameFilter, bool mustBePlayable) {
                LogExit exitPrint("ArmorFormSearchUtils.setup"sv);
                auto data = RE::TESDataHandler::GetSingleton();
                auto& list = data->GetFormArray(RE::FormType::Armor);
                const auto size = list.size();
                this->names.reserve(size);
                this->armors.reserve(size);
                for (std::uint32_t i = 0; i < size; i++) {
                    const auto form = list[i];
                    if (form && form->formType == RE::FormType::Armor) {
                        auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                        if (!armor) continue;
                        if (armor->templateArmor)// filter out predefined enchanted variants, to declutter the list
                            continue;
                        if (mustBePlayable && !!(armor->formFlags & RE::TESObjectARMO::RecordFlags::kNonPlayable))
                            continue;
                        std::string armorName;
                        {// get name
                            auto tfn = skyrim_cast<RE::TESFullName*>(armor);
                            if (tfn)
                                armorName = tfn->fullName.data();
                        }
                        if (armorName.empty())// skip nameless armor
                            continue;
                        if (!nameFilter.empty()) {
                            auto it = std::search(
                                armorName.begin(), armorName.end(),
                                nameFilter.begin(), nameFilter.end(),
                                [](char a, char b) { return toupper(a) == toupper(b); });
                            if (it == armorName.end())
                                continue;
                        }
                        this->armors.push_back(armor);
                        this->names.push_back(armorName.c_str());
                    }
                }
            }
            void clear() {
                this->names.clear();
                this->armors.clear();
            }
        } data;
        //
        //
        void Prep(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::BSFixedString filter,
                  bool mustBePlayable) {
            LogExit exitPrint("ArmorFormSearchUtils.Prep"sv);
            data.setup(filter.data(), mustBePlayable);
        }
        std::vector<RE::TESObjectARMO*> GetForms(RE::BSScript::IVirtualMachine* registry,
                                                 std::uint32_t stackId,
                                                 RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.GetForms"sv);
            std::vector<RE::TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            std::vector<RE::TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((RE::TESObjectARMO*) ptr);
            }
            return converted_result;
        }
        std::vector<RE::BSFixedString> GetNames(RE::BSScript::IVirtualMachine* registry,
                                                std::uint32_t stackId,
                                                RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.GetNames"sv);
            std::vector<RE::BSFixedString> result;
            auto& list = data.names;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(it->c_str());
            return result;
        }
        void Clear(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.Clear"sv);
            data.clear();
        }
    }// namespace ArmorFormSearchUtils
    namespace BodySlotListing {
        enum {
            kBodySlotMin = 30,
            kBodySlotMax = 61,
        };
        static struct {
            std::vector<std::int32_t> bodySlots;
            std::vector<std::string> armorNames;
            std::vector<RE::TESObjectARMO*> armors;
        } data;
        //
        void Clear(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.Clear"sv);
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
        }
        void Prep(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::BSFixedString name) {
            LogExit exitPrint("BodySlotListing.Prep"sv);
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
            //
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data());
                auto& armors = outfit.m_armors;
                for (std::uint8_t i = kBodySlotMin; i <= kBodySlotMax; i++) {
                    std::uint32_t mask = 1 << (i - kBodySlotMin);
                    for (auto it = armors.begin(); it != armors.end(); it++) {
                        RE::TESObjectARMO* armor = *it;
                        if (armor && (static_cast<std::uint32_t>(armor->GetSlotMask()) & mask)) {
                            data.bodySlots.push_back(i);
                            data.armors.push_back(armor);
                            {// name
                                auto pFullName = skyrim_cast<RE::TESFullName*>(armor);
                                if (pFullName)
                                    data.armorNames.emplace_back(pFullName->fullName.data());
                                else
                                    data.armorNames.emplace_back("");
                            }
                        }
                    }
                }
            } catch (std::out_of_range) {
                registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            }
        }
        std::vector<RE::TESObjectARMO*> GetArmorForms(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetArmorForms"sv);
            std::vector<RE::TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            std::vector<RE::TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((RE::TESObjectARMO*) ptr);
            }
            return converted_result;
        }
        std::vector<RE::BSFixedString> GetArmorNames(RE::BSScript::IVirtualMachine* registry,
                                                     std::uint32_t stackId,
                                                     RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetArmorNames"sv);
            std::vector<RE::BSFixedString> result;
            auto& list = data.armorNames;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(it->c_str());
            return result;
        }
        std::vector<std::int32_t> GetSlotIndices(RE::BSScript::IVirtualMachine* registry,
                                                 std::uint32_t stackId,
                                                 RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetSlotIndices"sv);
            std::vector<std::int32_t> result;
            auto& list = data.bodySlots;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            return result;
        }
    }// namespace BodySlotListing
    namespace StringSorts {
        std::vector<RE::BSFixedString> NaturalSort_ASCII(RE::BSScript::IVirtualMachine* registry,
                                                         std::uint32_t stackId,
                                                         RE::StaticFunctionTag*,
                                                         std::vector<RE::BSFixedString> arr,
                                                         bool descending) {
            LogExit exitPrint("StringSorts.NaturalSort_ASCII"sv);
            std::vector<RE::BSFixedString> result = arr;
            std::sort(
                result.begin(),
                result.end(),
                [descending](const RE::BSFixedString& x, const RE::BSFixedString& y) {
                    std::string a(x.data());
                    std::string b(y.data());
                    if (descending)
                        std::swap(a, b);
                    return cobb::utf8::naturalcompare(a, b) > 0;
                });
            return result;
        }

        template<typename T>
        std::vector<T*> NaturalSortPair_ASCII(
            RE::BSScript::IVirtualMachine* registry,
            std::uint32_t stackId,
            RE::StaticFunctionTag*,
            std::vector<RE::BSFixedString> arr,// Array of string
            std::vector<T*> second,            // Array of forms (T)
            bool descending) {
            LogExit exitPrint("StringSorts.NaturalSortPair_ASCII"sv);
            std::size_t size = arr.size();
            if (size != second.size()) {
                registry->TraceStack("The two arrays must be the same length.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
                return second;
            }
            //
            typedef std::pair<RE::BSFixedString, T*> _pair;
            std::vector<_pair> pairs;
            //
            std::vector<RE::BSFixedString> result;
            {// Copy input array into output array
                result.reserve(size);
                for (std::uint32_t i = 0; i < size; i++) {
                    pairs.emplace_back(arr[i], second[i]);
                }
            }
            std::sort(
                pairs.begin(),
                pairs.end(),
                [descending](const _pair& x, const _pair& y) {
                    std::string a(x.first.data());
                    std::string b(y.first.data());
                    if (descending)
                        std::swap(a, b);
                    return cobb::utf8::naturalcompare(a, b) > 0;
                });
            for (std::uint32_t i = 0; i < size; i++) {
                result.push_back(pairs[i].first);
                second[i] = pairs[i].second;
            }
            return second;
        }
    }// namespace StringSorts
    namespace Utility {
        std::uint32_t HexToInt32(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,
                                 RE::BSFixedString str) {
            LogExit exitPrint("Utility.HexToInt32"sv);
            const char* s = str.data();
            char* discard;
            return strtoul(s, &discard, 16);
        }
        RE::BSFixedString ToHex(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                                std::uint32_t value,
                                std::int32_t length) {
            LogExit exitPrint("Utility.ToHex"sv);
            if (length < 1) {
                registry->TraceStack(
                    "Cannot format a hexadecimal valueinteger to a negative number of digits. Defaulting to eight.",
                    stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
                length = 8;
            } else if (length > 8) {
                registry->TraceStack("Cannot format a hexadecimal integer longer than eight digits.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
                length = 8;
            }
            char hex[9];
            memset(hex, '0', sizeof(hex));
            hex[length] = '\0';
            while (value > 0 && length--) {
                std::uint8_t digit = value % 0x10;
                value /= 0x10;
                if (digit < 0xA) {
                    hex[length] = digit + '0';
                } else {
                    hex[length] = digit + 0x37;
                }
            }
            return hex;// passes through RE::BSFixedString constructor, which I believe caches the string, so returning local vars should be fine
        }
    }// namespace Utility
    //
    void AddArmorToOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                          RE::BSFixedString name,
                          RE::TESObjectARMO* armor_skse) {
        LogExit exitPrint("AddArmorToOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr, "Cannot add a None armor to an outfit.", registry, stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            outfit.m_armors.insert(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
    }
    bool ArmorConflictsWithOutfit(RE::BSScript::IVirtualMachine* registry,
                                  std::uint32_t stackId,
                                  RE::StaticFunctionTag*,

                                  RE::TESObjectARMO* armor_skse,
                                  RE::BSFixedString name) {
        LogExit exitPrint("ArmorConflictsWithOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        if (armor == nullptr) {
            registry->TraceStack("A None armor can't conflict with anything in an outfit.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            return false;
        }
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            return outfit.conflictsWith(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            return false;
        }
    }
    void CreateOutfit(RE::BSScript::IVirtualMachine* registry,
                      std::uint32_t stackId,
                      RE::StaticFunctionTag*,
                      RE::BSFixedString name) {
        LogExit exitPrint("CreateOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            service.addOutfit(name.data());
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("Invalid outfit name specified.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }

    void DeleteOutfit(RE::BSScript::IVirtualMachine* registry,
                      std::uint32_t stackId,
                      RE::StaticFunctionTag*,
                      RE::BSFixedString name) {
        LogExit exitPrint("DeleteOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.deleteOutfit(name.data());
    }
    std::vector<RE::TESObjectARMO*> GetOutfitContents(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*,

                                                      RE::BSFixedString name) {
        LogExit exitPrint("GetOutfitContents"sv);
        std::vector<RE::TESObjectARMO*> result;
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            auto& armors = outfit.m_armors;
            for (auto it = armors.begin(); it != armors.end(); ++it)
                result.push_back(*it);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
        std::vector<RE::TESObjectARMO*> converted_result;
        converted_result.reserve(result.size());
        for (const auto ptr : result) {
            converted_result.push_back((RE::TESObjectARMO*) ptr);
        }
        return converted_result;
    }
    bool GetOutfitFavoriteStatus(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,

                                 RE::BSFixedString name) {
        LogExit exitPrint("GetOutfitFavoriteStatus"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        bool result = false;
        try {
            auto& outfit = service.getOutfit(name.data());
            result = outfit.m_favorited;
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
        return result;
    }
    RE::BSFixedString GetSelectedOutfit(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor) {
        LogExit exitPrint("GetSelectedOutfit"sv);
        if (!actor)
            return RE::BSFixedString("");
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.currentOutfit(actor).m_name.c_str();
    }
    bool IsEnabled(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("IsEnabled"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.enabled;
    }

    bool IsExcludedPlugin(const std::string_view& filename) {
        // List of default Bethesda plugins
        static const std::vector<std::string_view> excludedPlugins = {
            "Skyrim.esm",
            "Update.esm",
            "Dawnguard.esm",
            "HearthFires.esm",
            "Dragonborn.esm",
            "Skyrim_VR.esm"
        };

        // Check if the filename is in the list
        for (const auto& plugin : excludedPlugins) {
            if (_stricmp(filename.data(), plugin.data()) == 0) {
                return true;
            }
        }

        return false;
    }

    namespace {
        struct ModEntryInfo {
            std::string filename;
            uint32_t loadOrderIndex;
            const RE::TESFile* filePtr; // Add file pointer
            std::unordered_map<std::string, RE::BGSOutfit*> outfitsMap;
        };

        std::unordered_map<std::string, ModEntryInfo> g_cachedModMap;
        std::vector<std::string> g_cachedModList;
        bool g_isModListCached = false;

        bool IsPrintableString(const char* str, size_t maxLen) {
            // First check if str is null
            if (!str) return false;

            try {
                size_t len = 0;
                while (str[len] && len < maxLen) {  // Use indexing which is sometimes safer
                    char c = str[len];
                    // Check if character is in printable ASCII range or common control chars
                    if ((c < 0x20 || c > 0x7E) && c != '\t' && c != '\n' && c != '\r') {
                        return false;
                    }
                    len++;
                }

                // Either we hit null terminator or reached max length
                return str[len] == '\0' || len < maxLen;
            }
            catch (...) {
                // If we hit any exception, the string is not safely readable
                return false;
            }
        }

        void CacheAllLoadedMods() {
            g_cachedModList.clear();
            g_cachedModMap.clear();

            auto dataHandler = RE::TESDataHandler::GetSingleton();
            if (!dataHandler) {
                // LOG(warn, "DataHandler is null");
                g_isModListCached = true;
                return;
            }

            std::unordered_set<std::string> uniqueMods;
            std::unordered_map<std::string, ModEntryInfo> userMods;

            auto outfitForms = dataHandler->GetFormArray<RE::BGSOutfit>();

            for (auto outfitForm : outfitForms) {
                const RE::TESFile* mod = outfitForm->GetFile(0);

                if (!mod)
                    continue;

                std::string filename = std::string(mod->GetFilename());

                // Skip excluded and duplicate plugins
                if (!IsExcludedPlugin(filename)) {
                    if (uniqueMods.insert(filename).second) {
                        ModEntryInfo entry;
                        entry.filename = filename;
                        entry.loadOrderIndex = mod->GetCompileIndex();
                        entry.filePtr = mod;
                        userMods[filename] = entry;

                        std::unordered_map<std::string, RE::BGSOutfit*> outfitMap;
                        std::string outfitName = REUtilities::get_editorID(outfitForm);
                        outfitMap[outfitName] = outfitForm;

                        entry.outfitsMap = outfitMap;

                        LOG(info, "Regular mod: {}", entry.filename);
                        LOG(info, "Done processing: {}, Load order {}", filename, mod->GetCompileIndex());
                    }
                    else if (userMods.contains(filename)) {
                        ModEntryInfo& entry = userMods[filename];
                        // Get outfit name
                        std::string outfitName = REUtilities::get_editorID(outfitForm);
                        if (outfitName.empty()) {
                            // Generate default name using form ID
                            outfitName = fmt::format("Outfit_{:X}", outfitForm->formID & 0xFFF);
                        }

                        entry.outfitsMap[outfitName] = outfitForm;
                    }
                }
            }

            g_cachedModList.reserve(userMods.size());
            for (const auto& mod : userMods) {
                g_cachedModList.push_back(mod.second.filename);
                g_cachedModMap[mod.second.filename] = mod.second;
                // LOG(info, "End result mod: {}", mod.filename);
            }

            std::sort(g_cachedModList.begin(), g_cachedModList.end(),
                [&userMods](std::string a, std::string b) {
                    auto aModInfo = userMods[a];
                    auto bModInfo = userMods[b];
                    return aModInfo.loadOrderIndex < bModInfo.loadOrderIndex;
                });

            g_isModListCached = true;
            LOG(info, "Total mod count: {}", g_cachedModList.size());
        }
    }

    std::vector<std::string> GetAllLoadedModsList(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*
    ) {
        std::vector<std::string> result;

        // Use cached mod list
        if (!g_isModListCached) {
            CacheAllLoadedMods();
        }

        result.reserve(g_cachedModList.size());
        for (const auto & modString : g_cachedModList) {
            result.push_back(modString);
        }

        ranges::sort(result);

        return result;
    }

    // Paginated list of outfits for a specific mod
    std::vector<std::string> GetAllLoadedOutfitsForMod(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*,
        std::string modName
    ) {
        LOG(info, "Grabbing all outfit records for {}", modName);
        std::vector<std::string> result;

        // Ensure mod list is cached
        if (!g_isModListCached) {
            CacheAllLoadedMods();
        }

        if (!g_cachedModMap.contains(modName)) {
            return result;
        }

        // Get all outfits for this mod
        auto& outfits = g_cachedModMap[modName].outfitsMap;

        result.reserve(outfits.size());
        for (const auto& outfitName : outfits | views::keys) {
            result.push_back(outfitName);
        }

        ranges::sort(result);

        LOG(info, "Mapped {} outfits for {}. Returning {} mods.", outfits.size(), modName, result.size());

        return result;
    }

    // Add a function to refresh the cache if needed (e.g., after mod installation during runtime)
    void RefreshModCache(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*
    ) {
        g_isModListCached = false;
        CacheAllLoadedMods();
    }

    std::vector<RE::BSFixedString> ListOutfits(RE::BSScript::IVirtualMachine* registry,
                                               std::uint32_t stackId,
                                               RE::StaticFunctionTag*,

                                               bool favoritesOnly) {
        LogExit exitPrint("ListOutfits"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        std::vector<RE::BSFixedString> result;
        std::vector<std::string> intermediate;
        service.getOutfitNames(intermediate, favoritesOnly);
        result.reserve(intermediate.size());
        for (auto it = intermediate.begin(); it != intermediate.end(); ++it)
            result.push_back(it->c_str());
        return result;
    }
    void RemoveArmorFromOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                               RE::BSFixedString name,
                               RE::TESObjectARMO* armor_skse) {
        LogExit exitPrint("RemoveArmorFromOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr, "Cannot remove a None armor from an outfit.", registry, stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            outfit.m_armors.erase(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
    }
    void RemoveConflictingArmorsFrom(RE::BSScript::IVirtualMachine* registry,
                                     std::uint32_t stackId,
                                     RE::StaticFunctionTag*,

                                     RE::TESObjectARMO* armor_skse,
                                     RE::BSFixedString name) {
        LogExit exitPrint("RemoveConflictingArmorsFrom"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr,
                            "A None armor can't conflict with anything in an outfit.",
                            registry,
                            stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            auto& armors = outfit.m_armors;
            std::vector<RE::TESObjectARMO*> conflicts;
            const auto candidateMask = armor->GetSlotMask();
            for (auto it = armors.begin(); it != armors.end(); ++it) {
                RE::TESObjectARMO* existing = *it;
                if (existing) {
                    const auto mask = existing->GetSlotMask();
                    if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(candidateMask))
                        != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                        conflicts.push_back(existing);
                }
            }
            for (auto it = conflicts.begin(); it != conflicts.end(); ++it)
                armors.erase(*it);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }
    bool RenameOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                      RE::BSFixedString name,
                      RE::BSFixedString changeTo) {
        LogExit exitPrint("RenameOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            service.renameOutfit(name.data(), changeTo.data());
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("The desired name is invalid.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        } catch (ArmorAddonOverrideService::name_conflict) {
            registry->TraceStack("The desired name is taken.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        }
        return true;
    }
    void SetOutfitFavoriteStatus(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,

                                 RE::BSFixedString name,
                                 bool favorite) {
        LogExit exitPrint("SetOutfitFavoriteStatus"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setFavorite(name.data(), favorite);
    }
    bool OutfitExists(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                      RE::BSFixedString name) {
        LogExit exitPrint("OutfitExists"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.hasOutfit(name.data());
    }
    void OverwriteOutfit(RE::BSScript::IVirtualMachine* registry,
                         std::uint32_t stackId,
                         RE::StaticFunctionTag*,
                         RE::BSFixedString name,
                         std::vector<RE::TESObjectARMO*> armors) {
        LogExit exitPrint("OverwriteOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOrCreateOutfit(name.data());
            outfit.m_armors.clear();
            auto count = armors.size();
            for (std::uint32_t i = 0; i < count; i++) {
                RE::TESObjectARMO* ptr = nullptr;
                ptr = armors.at(i);
                if (ptr)
                    outfit.m_armors.insert(ptr);
            }
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("Invalid outfit name specified.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }

    // returns the status, 1 for success, 0 for failure.
    uint32_t AddOutfitFromModToOutfitList(RE::BSScript::IVirtualMachine* registry,
                      std::uint32_t stackId,
                      RE::StaticFunctionTag*,
                      std::string modName,
                      std::string formEditorID
    ) {
        // Ensure mod list is cached
        if (!g_isModListCached) {
            CacheAllLoadedMods();
        }

        auto modIt = g_cachedModMap.find(modName);

        if (modIt == g_cachedModMap.end()) {
            return 0;
        }

        if (modIt->second.outfitsMap.empty()) {
            // attempt to load up outfits
            auto outfits = GetAllLoadedOutfitsForMod(registry, stackId, nullptr, modName);
            auto modItRetry = g_cachedModMap.find(modName);

            if (outfits.empty() || modItRetry->second.outfitsMap.empty()) {
                LOG(critical, "Failed to load any outfits for {}", modName);
                return 0;
            }
        }

        // Get outfits for this mod
        auto outfit = modIt->second.outfitsMap.find(formEditorID);

        if (outfit == modIt->second.outfitsMap.end()) {
            return 0;
        }

        std::vector<RE::TESObjectARMO*> outfitArmors = REUtilities::OutfitToArmorList(outfit->second);

        try {
            OverwriteOutfit(registry, stackId, nullptr, outfit->first, outfitArmors);
            return 1;
        }
        catch (const std::exception& e) {
            LOG(critical, "Failed to add outfit {} from mod {}", formEditorID, modName);
            return 0;
        }
    }

    // Function to add all outfits from a mod to the outfit list
    // Returns the total number of outfits added
    uint32_t AddAllOutfitsFromModToOutfitList(RE::BSScript::IVirtualMachine* registry,
                          std::uint32_t stackId,
                          RE::StaticFunctionTag*,
                          std::string modName)
    {
        // Ensure mod list is cached
        if (!g_isModListCached) {
            CacheAllLoadedMods();
        }

        auto modIt = g_cachedModMap.find(modName);

        if (modIt == g_cachedModMap.end()) {
            LOG(critical, "Could not find mod {} in cached mod map", modName);
            return 0;
        }

        if (modIt->second.outfitsMap.empty()) {
            // attempt to load up outfits
            auto outfits = GetAllLoadedOutfitsForMod(registry, stackId, nullptr, modName);
            auto modItRetry = g_cachedModMap.find(modName);

            if (outfits.empty() || modItRetry->second.outfitsMap.empty()) {
                LOG(critical, "Failed to load any outfits for {}", modName);
                return 0;
            }
        }

        unordered_map<string, RE::BGSOutfit*> outfitMap = modIt->second.outfitsMap;
        uint32_t addedCount = 0;

        // Iterate through all outfits in the mod
        for (const auto& [formEditorID, outfitPtr] : outfitMap) {
            // Skip if outfit is null
            if (!outfitPtr) {
                LOG(critical, "Skipping null outfit with formEditorID: {}", formEditorID);
                continue;
            }

            // Call the existing function to add each outfit
            uint32_t result = AddOutfitFromModToOutfitList(registry, stackId, nullptr, modName, formEditorID);

            // If result equal to or above 1, the outfit was successfully added
            if (result >= 1) {
                addedCount++;
                LOG(critical,"Successfully added outfit {} from mod {}", formEditorID, modName);
            }
            else {
                LOG(critical,"Failed to add outfit {} from mod {}", formEditorID, modName);
            }
        }

        LOG(info,"Added {} outfits to {}.", addedCount, modName);

        // Return the number of outfits successfully added
        return addedCount;
    }

    void SetEnabled(RE::BSScript::IVirtualMachine* registry,
                    std::uint32_t stackId,
                    RE::StaticFunctionTag*,
                    bool state) {
        LogExit exitPrint("SetEnabled"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setEnabled(state);
    }

    std::uint32_t GetPlayerInventoryManagementMode(RE::BSScript::IVirtualMachine* registry,
                std::uint32_t stackId,
                RE::StaticFunctionTag*) {
        LogExit exitPrint("GetPlayerInventoryManagementMode"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return static_cast<std::uint32_t>(service.getPlayerInventoryManagementMode());
    }

    void SetPlayerInventoryManagementMode(RE::BSScript::IVirtualMachine* registry,
                    std::uint32_t stackId,
                    RE::StaticFunctionTag*,
                    std::uint32_t mode) {
        LogExit exitPrint("SetPlayerInventoryManagementMode"sv);
        if (mode <= static_cast<std::uint32_t>(InventoryManagementMode::Immersive)) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setPlayerInventoryManagementMode(static_cast<InventoryManagementMode>(mode));
        } else {
            LOG(critical, "Invalid inventory management mode requested: {}. Cannot set Player inventory management as such mode.", mode);
        }
    }

    std::uint32_t GetNPCInventoryManagementMode(RE::BSScript::IVirtualMachine* registry,
                std::uint32_t stackId,
                RE::StaticFunctionTag*) {
        LogExit exitPrint("GetPlayerInventoryManagementMode"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return static_cast<std::uint32_t>(service.getNPCInventoryManagementMode());
    }

    void SetNPCInventoryManagementMode(RE::BSScript::IVirtualMachine* registry,
                    std::uint32_t stackId,
                    RE::StaticFunctionTag*,
                    std::uint32_t mode) {
        LogExit exitPrint("SetNPCInventoryManagementMode"sv);
        if (mode <= static_cast<std::uint32_t>(InventoryManagementMode::Immersive)) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setNPCInventoryManagementMode(static_cast<InventoryManagementMode>(mode));
        } else {
            LOG(critical, "Invalid inventory management mode requested: {}. Cannot set NPC inventory management as such mode.", mode);
        }
    }

    void SetSelectedOutfit(RE::BSScript::IVirtualMachine* registry,
                           std::uint32_t stackId,
                           RE::StaticFunctionTag*,
                           RE::Actor* actor,
                           RE::BSFixedString name) {
        LogExit exitPrint("SetSelectedOutfit"sv);
        if (!actor)
            return;
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setOutfit(name.data(), actor);
    }
    void AddActor(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::Actor* target) {
        LogExit exitPrint("AddActor"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.addActor(target);
    }
    void RemoveActor(RE::BSScript::IVirtualMachine* registry,
                     std::uint32_t stackId,
                     RE::StaticFunctionTag*,
                     RE::Actor* target) {
        LogExit exitPrint("RemoveActor"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.removeActor(target);
    }
    std::vector<RE::Actor*> ListActors(RE::BSScript::IVirtualMachine* registry,
                                       std::uint32_t stackId,
                                       RE::StaticFunctionTag*) {
        LogExit exitPrint("ListActors"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        auto actors = service.listActors();
        std::vector<RE::Actor*> actorVec;
        for (auto& actor : actors) {
            if (!actor) continue;
#if _DEBUG
            LOG(debug, "INNER: Actor {} has refcount {}", actor->GetDisplayFullName(), actor->QRefCount());
#endif
            if (actor->QRefCount() == 1) {
                LOG(warn, "ListActors will return an actor {} with refcount of 1. This may crash.", actor->GetDisplayFullName());
            }
            actorVec.push_back(actor);
        }
        std::sort(
            actorVec.begin(),
            actorVec.end(),
            [](const RE::Actor* x, const RE::Actor* y) {
                return x < y;
            });
#if _DEBUG
        for (const auto& actor : actorVec) {
            LOG(debug, "Actor {} has refcount {}", actor->GetDisplayFullName(), actor->QRefCount());
        }
#endif
        return actorVec;
    }

    bool HasActor(RE::BSScript::IVirtualMachine* registry,
                                       std::uint32_t stackId,
                                       RE::StaticFunctionTag*,
                                       RE::Actor* target) {
        LogExit exitPrint("ListActors"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        auto actors = service.listActors();
        return actors.contains(target);
    }

    std::vector<std::uint32_t> GetAutoSwitchGenericLocationArray(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*) {
        LogExit exitPrint("GetAutoSwitchLocationArray"sv);
        std::vector<std::uint32_t> result;
        for (LocationType i : {
             LocationType::World,
             LocationType::WorldNight,
             LocationType::WorldSnow,
             LocationType::WorldRain,
             LocationType::WorldInterior,

             LocationType::Town,
             LocationType::TownNight,
             LocationType::TownSnow,
             LocationType::TownRain,
             LocationType::TownInterior,

             LocationType::City,
             LocationType::CityNight,
             LocationType::CitySnow,
             LocationType::CityRain,
             LocationType::CityInterior,
        }) {
            result.push_back(static_cast<std::uint32_t>(i));
        }
        return result;
    }

    std::vector<std::uint32_t> GetAutoSwitchSpecificLocationArray(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*) {
        LogExit exitPrint("GetAutoSwitchLocationArray"sv);
        std::vector<std::uint32_t> result;
        for (LocationType i : {
                 LocationType::Dungeon,
                 LocationType::PlayerHome,
                 LocationType::Inn,
                 LocationType::Store,
                 LocationType::GuildHall,
                 LocationType::Castle,
                 LocationType::Temple,
                 LocationType::Farm,
                 LocationType::Jail,
                 LocationType::Military
        }) {
            result.push_back(static_cast<std::uint32_t>(i));
        }
        return result;
    }

    std::vector<std::uint32_t> GetAutoSwitchActionBasedLocationArray(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*) {
        LogExit exitPrint("GetAutoSwitchLocationArray"sv);
        std::vector<std::uint32_t> result;
        for (LocationType i : {
            LocationType::Combat,
            LocationType::InWater,
            LocationType::Sleeping,
            LocationType::Swimming,
            LocationType::Mounting,
        }) {
            result.push_back(static_cast<std::uint32_t>(i));
        }
        return result;
    }

    std::optional<LocationType> identifyLocation(RE::BGSLocation* location, RE::TESWeather* weather, RE::Actor* target) {
        LogExit exitPrint("identifyLocation"sv);
        // Just a helper function to classify a location.
        // TODO: Think of a better place than this since we're not exposing it to Papyrus.
        auto& service = ArmorAddonOverrideService::GetInstance();

        // Collect weather information.
        WeatherFlags weather_flags;
        if (weather) {
            weather_flags.snowy = weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kSnow);
            weather_flags.rainy = weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy);
        }

        // Collect location keywords
        std::unordered_set<std::string> keywords;
        keywords.reserve(20);
        while (location) {
            std::uint32_t max = location->GetNumKeywords();
            for (std::uint32_t i = 0; i < max; i++) {
                RE::BGSKeyword* keyword = location->GetKeywordAt(i).value();
                /*
                char message[100];
                LOG(info, "SOS: Location has Keyword %s", keyword->GetFormEditorID());
                sprintf(message, "SOS: Location has keyword %s", keyword->GetFormEditorID());
                RE::DebugNotification(message, nullptr, false);
                */
                keywords.emplace(keyword->GetFormEditorID());
            }
            location = location->parentLoc;
        }

        return service.checkLocationType(keywords, weather_flags, REUtilities::CurrentGameDayPart(), target);
    }

    std::uint32_t IdentifyLocationType(RE::BSScript::IVirtualMachine* registry,
                                       std::uint32_t stackId,
                                       RE::StaticFunctionTag*,
                                       RE::BGSLocation* location_skse,
                                       RE::TESWeather* weather_skse,
                                       RE::Actor* target) {
        LogExit exitPrint("IdentifyLocationType"sv);
        // NOTE: Identify the location for Papyrus. In the event no location is identified, we lie to Papyrus and say "World".
        //       Therefore, Papyrus cannot assume that locations returned have an outfit assigned, at least not for "World".
        return static_cast<std::uint32_t>(identifyLocation(location_skse,weather_skse, target).value_or(LocationType::World));
    }

    void SetOutfitsUsingLocationRaw(RE::BGSLocation* location_skse,
                                RE::TESWeather* weather_skse) {
        LogExit exitPrint("SetOutfitsUsingLocation"sv);
        // NOTE: Location can be NULL.
        auto& service = ArmorAddonOverrideService::GetInstance();
        auto actors = service.listActors();

        for (auto& actor : actors) {
            if (!actor || !actor->Is3DLoaded()) continue;

            auto location = identifyLocation(location_skse, weather_skse, actor);
            // Debug notifications for location classification.
            /*
            const char* locationName = locationTypeStrings[static_cast<std::uint32_t>(location)];
            char message[100];
            sprintf_s(message, "SOS: This location is a %s.", locationName);
            RE::DebugNotification(message, nullptr, false);
            */
            if (location.has_value()) {
                service.setOutfitUsingLocation(location.value(), actor);
            }
        }
    }

    void SetOutfitsUsingLocation(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                                RE::BGSLocation* location_skse,
                                RE::TESWeather* weather_skse) {
        SetOutfitsUsingLocationRaw(location_skse, weather_skse);
    }

    void SetLocationOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                           RE::Actor* actor,
                           std::uint32_t location,
                           RE::BSFixedString name) {
        LogExit exitPrint("SetLocationOutfit"sv);
        if (!actor)
            return;
        if (strcmp(name.data(), "") == 0) {
            // Location outfit assignment is never allowed to be empty string. Use unset instead.
            return;
        }
        return ArmorAddonOverrideService::GetInstance()
            .setLocationOutfit(LocationType(location), name.data(), actor);
    }
    void UnsetLocationOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                             RE::Actor* actor,
                             std::uint32_t location) {
        LogExit exitPrint("UnsetLocationOutfit"sv);
        if (!actor)
            return;
        return ArmorAddonOverrideService::GetInstance()
            .unsetLocationOutfit(LocationType(location), actor);
    }
    RE::BSFixedString GetLocationOutfit(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor,
                                        std::uint32_t location) {
        LogExit exitPrint("GetLocationOutfit"sv);
        if (!actor)
            return RE::BSFixedString("");
        auto outfit = ArmorAddonOverrideService::GetInstance()
                          .getLocationOutfit(LocationType(location), actor);
        if (outfit.has_value()) {
            return RE::BSFixedString(outfit.value().c_str());
        } else {
            // Empty string means "no outfit assigned" for this location type.
            return RE::BSFixedString("");
        }
    }

    void AutoOutfitSwitchStateReset(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        auto& autoSwitchService = AutoOutfitSwitchService::GetSingleton();
        autoSwitchService.StateReset();
    }

    bool ExportSettings(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("ExportSettings"sv);
        std::string outputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitEquipmentSystemNGData.json";
        auto& service = ArmorAddonOverrideService::GetInstance();
        proto::OutfitSystem data = service.save();
        std::string output;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        google::protobuf::util::MessageToJsonString(data, &output, options);
        std::ofstream file(outputFile);
        if (file) {
            file << output;
        } else {
            RE::DebugNotification("Failed to open config for writing", nullptr, false);
            return false;
        }
        if (file.good()) {
            std::string message = "Wrote JSON config to " + outputFile;
            RE::DebugNotification(message.c_str(), nullptr, false);
            return true;
        } else {
            RE::DebugNotification("Failed to write config", nullptr, false);
            return false;
        }
    }
    bool ImportSettings(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("ImportSettings"sv);
        std::string inputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitEquipmentSystemNGData.json";
        std::ifstream file(inputFile);
        if (!file) {
            RE::DebugNotification("Failed to open config for reading", nullptr, false);
            return false;
        }
        std::stringstream input;
        input << file.rdbuf();
        if (!file.good()) {
            RE::DebugNotification("Failed to read config data", nullptr, false);
            return false;
        }
        proto::OutfitSystem data;
        auto status = google::protobuf::util::JsonStringToMessage(input.str(), &data);
        if (!status.ok()) {
            RE::DebugNotification("Failed to parse config data. Invalid syntax.", nullptr, false);
            return false;
        }
        auto& service = ArmorAddonOverrideService::GetInstance();
        service = ArmorAddonOverrideService(data, SKSE::GetSerializationInterface());
        std::string message = "Read JSON config from " + inputFile;
        RE::DebugNotification(message.c_str(), nullptr, false);
        return true;
    }
}// namespace OutfitSystem

bool OutfitSystem::RegisterPapyrus(RE::BSScript::IVirtualMachine* registry) {
    registry->RegisterFunction("GetOutfitNameMaxLength",
                               "SkyrimOutfitEquipmentSystemNativeFuncs",
                               GetOutfitNameMaxLength);
    registry->RegisterFunction(
        "GetOutfitNameMaxLength",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetOutfitNameMaxLength,
        true);
    registry->RegisterFunction(
        "GetCarriedArmor",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetCarriedArmor);
    registry->RegisterFunction(
        "GetWornItems",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetWornItems);
    registry->RegisterFunction(
        "RefreshArmorFor",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RefreshArmorFor);
    registry->RegisterFunction(
        "RefreshArmorForAllConfiguredActors",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RefreshArmorForAllConfiguredActors);
    registry->RegisterFunction(
        "ActorNearPC",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ActorsNearPC);
    //
    {// armor form search utils
        registry->RegisterFunction(
            "PrepArmorSearch",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            ArmorFormSearchUtils::Prep);
        registry->RegisterFunction(
            "GetArmorSearchResultForms",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            ArmorFormSearchUtils::GetForms);
        registry->RegisterFunction(
            "GetArmorSearchResultNames",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            ArmorFormSearchUtils::GetNames);
        registry->RegisterFunction(
            "ClearArmorSearch",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            ArmorFormSearchUtils::Clear);
    }
    {// body slot data
        registry->RegisterFunction(
            "PrepOutfitBodySlotListing",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            BodySlotListing::Prep);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingArmorForms",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            BodySlotListing::GetArmorForms);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingArmorNames",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            BodySlotListing::GetArmorNames);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingSlotIndices",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            BodySlotListing::GetSlotIndices);
        registry->RegisterFunction(
            "ClearOutfitBodySlotListing",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            BodySlotListing::Clear);
    }
    {// string sorts
        registry->RegisterFunction(
            "NaturalSort_ASCII",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            StringSorts::NaturalSort_ASCII,
            true);
        registry->RegisterFunction(
            "NaturalSortPairArmor_ASCII",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            StringSorts::NaturalSortPair_ASCII<RE::TESObjectARMO>,
            true);
    }
    {// Utility
        registry->RegisterFunction(
            "HexToInt32",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            Utility::HexToInt32,
            true);
        registry->RegisterFunction(
            "ToHex",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            Utility::ToHex,
            true);
    }
    //
    registry->RegisterFunction(
        "AddArmorToOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        AddArmorToOutfit);
    registry->RegisterFunction(
        "ArmorConflictsWithOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ArmorConflictsWithOutfit);
    registry->RegisterFunction(
        "CreateOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        CreateOutfit);
    registry->RegisterFunction(
        "DeleteOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        DeleteOutfit);
    registry->RegisterFunction(
        "GetOutfitContents",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetOutfitContents);
    registry->RegisterFunction(
        "GetOutfitFavoriteStatus",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetOutfitFavoriteStatus);
    registry->RegisterFunction(
        "SetOutfitFavoriteStatus",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetOutfitFavoriteStatus);
    registry->RegisterFunction(
        "IsEnabled",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        IsEnabled);
    registry->RegisterFunction(
        "GetSelectedOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetSelectedOutfit);
    registry->RegisterFunction(
        "ListOutfits",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ListOutfits);
    registry->RegisterFunction(
        "RemoveArmorFromOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RemoveArmorFromOutfit);
    registry->RegisterFunction(
        "RemoveConflictingArmorsFrom",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RemoveConflictingArmorsFrom);
    registry->RegisterFunction(
        "RenameOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RenameOutfit);
    registry->RegisterFunction(
        "OutfitExists",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        OutfitExists);
    registry->RegisterFunction(
        "OverwriteOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        OverwriteOutfit);
    registry->RegisterFunction(
        "SetEnabled",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetEnabled);
    registry->RegisterFunction(
        "SetSelectedOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetSelectedOutfit);
    registry->RegisterFunction(
        "AddActor",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        AddActor);
    registry->RegisterFunction(
        "RemoveActor",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        RemoveActor);
    registry->RegisterFunction(
        "ListActors",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ListActors);
    registry->RegisterFunction(
            "HasActor",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            HasActor);
    registry->RegisterFunction(
        "GetAutoSwitchGenericLocationArray",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetAutoSwitchGenericLocationArray);
    registry->RegisterFunction(
        "GetAutoSwitchSpecificLocationArray",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetAutoSwitchSpecificLocationArray);
    registry->RegisterFunction(
        "GetAutoSwitchActionBasedLocationArray",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetAutoSwitchActionBasedLocationArray);
    registry->RegisterFunction(
        "IdentifyLocationType",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        IdentifyLocationType);
    registry->RegisterFunction(
        "SetOutfitsUsingLocation",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetOutfitsUsingLocation);
    registry->RegisterFunction(
        "SetLocationOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetLocationOutfit);
    registry->RegisterFunction(
        "UnsetLocationOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        UnsetLocationOutfit);
    registry->RegisterFunction(
        "GetLocationOutfit",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetLocationOutfit);
    registry->RegisterFunction(
        "ExportSettings",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ExportSettings);
    registry->RegisterFunction(
        "ImportSettings",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        ImportSettings);
    registry->RegisterFunction(
        "GetAllLoadedModsList",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetAllLoadedModsList);
    registry->RegisterFunction(
        "GetAllLoadedOutfitsForMod",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetAllLoadedOutfitsForMod);
    registry->RegisterFunction(
        "AddOutfitFromModToOutfitList",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        AddOutfitFromModToOutfitList);
    registry->RegisterFunction(
            "AddAllOutfitsFromModToOutfitList",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            AddAllOutfitsFromModToOutfitList);
    registry->RegisterFunction(
                "AutoOutfitSwitchStateReset",
                "SkyrimOutfitEquipmentSystemNativeFuncs",
                AutoOutfitSwitchStateReset);
    registry->RegisterFunction(
            "GetPlayerInventoryManagementMode",
            "SkyrimOutfitEquipmentSystemNativeFuncs",
            GetPlayerInventoryManagementMode);
    registry->RegisterFunction(
        "SetPlayerInventoryManagementMode",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetPlayerInventoryManagementMode);
    registry->RegisterFunction(
        "GetNPCInventoryManagementMode",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        GetNPCInventoryManagementMode);
    registry->RegisterFunction(
        "SetNPCInventoryManagementMode",
        "SkyrimOutfitEquipmentSystemNativeFuncs",
        SetNPCInventoryManagementMode);
    return true;
}