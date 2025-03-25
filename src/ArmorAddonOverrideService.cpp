#include "ArmorAddonOverrideService.h"

#include <google/protobuf/util/json_util.h>

#include "Forms.h"

#ifndef SKYRIMOUTFITEQUIPMENTSYSTEMNG_INCLUDE_RE_REAUGMENTS_H
#define SKYRIMOUTFITEQUIPMENTSYSTEMNG_INCLUDE_RE_REAUGMENTS_H

#endif//SKYRIMOUTFITEQUIPMENTSYSTEMNG_INCLUDE_RE_REAUGMENTS_H

void _assertWrite(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::save_error(err);
}
void _assertRead(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::load_error(err);
}

Outfit::Outfit(const proto::Outfit& proto, const SKSE::SerializationInterface* intfc) {
    m_name = proto.name();
    for (const auto& formID : proto.armors()) {
        RE::TESObjectARMO* armor = skyrim_cast<RE::TESObjectARMO*>(Forms::ParseFormString(formID));
        m_armors.insert(armor);
    }
    m_favorited = proto.is_favorite();
}

bool Outfit::conflictsWith(RE::TESObjectARMO* test) const {
    if (!test)
        return false;
    const auto mask = static_cast<uint32_t>(test->GetSlotMask());
    for (auto it = m_armors.cbegin(); it != m_armors.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor)
            if ((mask & static_cast<uint32_t>(armor->GetSlotMask()))
                != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                return true;
    }
    return false;
}
bool Outfit::hasShield() const {
    auto& list = m_armors;
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor) {
            if ((armor->formFlags & RE::TESObjectARMO::RecordFlags::kShield) != 0)
                return true;
        }
    }
    return false;
};

proto::Outfit Outfit::save() const {
    proto::Outfit out;
    out.set_name(m_name);
    for (const auto& armor : m_armors) {
        if (armor)
            out.add_armors(Forms::GetFormString(armor));
    }
    out.set_is_favorite(m_favorited);
    return out;
}

ArmorAddonOverrideService::ArmorAddonOverrideService(const proto::OutfitSystem& data, const SKSE::SerializationInterface* intfc) {
    try {
        std::string protoData;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        auto status = google::protobuf::util::MessageToJsonString(data, &protoData, options);

        // LOG(info, "Reading the following stored SKSE data:\n {}", protoData);

        // Extract data from the protobuf struct.
        enabled = data.enabled();
        playerInventoryManagementMode = static_cast<InventoryManagementMode>(data.player_inventory_management_mode());
        npcInventoryManagementMode = static_cast<InventoryManagementMode>(data.npc_inventory_management_mode());
        std::map<RE::Actor*, ActorOutfitAssignments> actorOutfitAssignmentsLocal;
        auto pc = RE::PlayerCharacter::GetSingleton();

        for (const auto& actorAssn : data.actor_outfit_assignments()) {
            // Lookup the actor
            std::uint64_t handle;
            std::string actorRefFormString = actorAssn.first;

            RE::Actor* actor = skyrim_cast<RE::Actor*>(Forms::ParseFormString(actorRefFormString));

            ActorOutfitAssignments assignments;
            assignments.currentOutfitName =
                cobb::istring(actorAssn.second.current_outfit_name().data(), actorAssn.second.current_outfit_name().size());
            for (const auto& locOutfitData : actorAssn.second.location_based_outfits()) {
                assignments.locationOutfits.emplace(
                    static_cast<LocationType>(locOutfitData.first),
                    cobb::istring(locOutfitData.second.data(),
                    locOutfitData.second.size())
                );
            }

            actorOutfitAssignmentsLocal[actor] = assignments;
        }

        // if the player character is not inside, add it
        if (actorOutfitAssignmentsLocal.count(pc) == 0) {
            LOG(info, "PC player does NOT exists inside the protocol");
            actorOutfitAssignmentsLocal[pc] = ActorOutfitAssignments();
        }
        else LOG(info, "PC player already exists inside the protocol");

        actorOutfitAssignments = actorOutfitAssignmentsLocal;
        for (const auto& outfitData : data.outfits()) {
            outfits.emplace(std::piecewise_construct,
                            std::forward_as_tuple(cobb::istring(outfitData.name().data(), outfitData.name().size())),
                            std::forward_as_tuple(outfitData, intfc));
        }
    }
    catch (const std::exception &e) {
        // print the exception
        LOG(info, "Exception initializing the armor override service, %s");
    }
}

void ArmorAddonOverrideService::_validateNameOrThrow(const char* outfitName) {
    if (strcmp(outfitName, g_noOutfitName) == 0)
        throw bad_name("Outfits can't use a blank name.");
    if (strlen(outfitName) > ce_outfitNameMaxLength)
        throw bad_name("The outfit's name is too long.");
}
//
Outfit& ArmorAddonOverrideService::getOutfit(const char* name) {
    return outfits.at(name);
}
Outfit& ArmorAddonOverrideService::getOrCreateOutfit(const char* name) {
    _validateNameOrThrow(name);
    auto created = outfits.emplace(name, name);
    return created.first->second;
}
//
void ArmorAddonOverrideService::addOutfit(const char* name) {
    _validateNameOrThrow(name);
    outfits.emplace(name, name);
}

void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
    _validateNameOrThrow(name);
    auto& created = outfits.emplace(name, name).first->second;
    for (auto it = armors.begin(); it != armors.end(); ++it) {
        auto armor = *it;
        if (armor)
            created.m_armors.insert(armor);
    }
}

Outfit& ArmorAddonOverrideService::currentOutfit(RE::Actor* target) {
    if (!actorOutfitAssignments.contains(target)) return g_noOutfit;
    if (actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName) return g_noOutfit;
    auto outfit = outfits.find(actorOutfitAssignments.at(target).currentOutfitName);
    if (outfit == outfits.end()) return g_noOutfit;
    return outfit->second;
}

bool ArmorAddonOverrideService::hasOutfit(const char* name) const {
    return outfits.contains(name);
}

void ArmorAddonOverrideService::deleteOutfit(const char* name) {
    outfits.erase(name);
    for (auto& assn : actorOutfitAssignments) {
        if (assn.second.currentOutfitName == name)
            assn.second.currentOutfitName = g_noOutfitName;
        // If the outfit is assigned as a location outfit, remove it there as well.
        for (auto it = assn.second.locationOutfits.begin(); it != assn.second.locationOutfits.end(); ++it) {
            if (it->second == name) {
                assn.second.locationOutfits.erase(it);
                break;
            }
        }
    }
}

void ArmorAddonOverrideService::setFavorite(const char* name, bool favorite) {
    auto outfit = outfits.find(name);
    if (outfit != outfits.end())
        outfit->second.m_favorited = favorite;
}

void ArmorAddonOverrideService::modifyOutfit(const char* name,
                                             std::vector<RE::TESObjectARMO*>& add,
                                             std::vector<RE::TESObjectARMO*>& remove,
                                             bool createIfMissing) {
    try {
        Outfit& target = getOutfit(name);
        for (auto it = add.begin(); it != add.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.m_armors.insert(armor);
        }
        for (auto it = remove.begin(); it != remove.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.m_armors.erase(armor);
        }
    } catch (std::out_of_range) {
        if (createIfMissing) {
            addOutfit(name);
            modifyOutfit(name, add, remove);
        }
    }
}
void ArmorAddonOverrideService::renameOutfit(const char* oldName, const char* newName) {
    _validateNameOrThrow(newName);
    if (outfits.contains(newName)) throw name_conflict("");
    auto outfitNode = outfits.extract(oldName);
    if (outfitNode.empty()) throw std::out_of_range("");
    outfitNode.key() = newName;
    outfitNode.mapped().m_name = newName;
    outfits.insert(std::move(outfitNode));
    for (auto& assignment : actorOutfitAssignments) {
        if (assignment.second.currentOutfitName == oldName)
            assignment.second.currentOutfitName = newName;
        // If the outfit is assigned as a location outfit, remove it there as well.
        for (auto& locationOutfit : assignment.second.locationOutfits) {
            if (locationOutfit.second == oldName) {
                assignment.second.locationOutfits[locationOutfit.first] = newName;
                break;
            }
        }
    }
}
void ArmorAddonOverrideService::setOutfit(const char* name, RE::Actor* target) {
    if (!actorOutfitAssignments.contains(target)) return;
    if (strcmp(name, g_noOutfitName) == 0) {
        actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
        return;
    }
    try {
        getOutfit(name);
        actorOutfitAssignments.at(target).currentOutfitName = name;
    } catch (std::out_of_range) {
        LOG(info,
            "ArmorAddonOverrideService: Tried to set non-existent outfit %s as active. Switching the system off for now.",
            name);
        actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
    }
}

void ArmorAddonOverrideService::addActor(RE::Actor* target) {
    if (actorOutfitAssignments.count(target) == 0)
        actorOutfitAssignments.emplace(target, ActorOutfitAssignments());
}

void ArmorAddonOverrideService::removeActor(RE::Actor* target) {
    LOG(critical,"Removing actor {}", target->GetName());
    actorOutfitAssignments.erase(target);
}

std::unordered_set<RE::Actor*> ArmorAddonOverrideService::listActors() {
    std::unordered_set<RE::Actor*> actors;
    for (auto& assignment : actorOutfitAssignments) {
        actors.insert(assignment.first);
    }
    return actors;
}

void ArmorAddonOverrideService::setOutfitUsingLocation(LocationType location, RE::Actor* target) {
    if (actorOutfitAssignments.count(target) == 0) {
        LOG(info, "No target found, cannot set outfit using location!");
        return;
    }
    auto it = actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != actorOutfitAssignments.at(target).locationOutfits.end()) {
        setOutfit(it->second.c_str(), target);
    }
}

void ArmorAddonOverrideService::setLocationOutfit(LocationType location, const char* name, RE::Actor* target) {
    if (actorOutfitAssignments.count(target) == 0) {
        LOG(info, "No target found, cannot set location outfit!");
        return;
    }
    if (!std::string(name).empty()) {// Can never set outfit to the "" outfit. Use unsetLocationOutfit instead.
        actorOutfitAssignments.at(target).locationOutfits[location] = name;
    }
}

void ArmorAddonOverrideService::unsetLocationOutfit(LocationType location, RE::Actor* target) {
    if (actorOutfitAssignments.count(target) == 0)
        return;
    actorOutfitAssignments.at(target).locationOutfits.erase(location);
}

std::optional<cobb::istring> ArmorAddonOverrideService::getLocationOutfit(LocationType location, RE::Actor* target) {
    if (actorOutfitAssignments.count(target) == 0)
        return std::optional<cobb::istring>();
    ;
    auto it = actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != actorOutfitAssignments.at(target).locationOutfits.end()) {
        return std::optional<cobb::istring>(it->second);
    } else {
        return std::optional<cobb::istring>();
    }
}

#define CHECK_LOCATION(TYPE, CHECK_CODE)                                                             \
    if (actorOutfitAssignments.at(target).locationOutfits.count(LocationType::TYPE) && (CHECK_CODE)) \
        return std::optional<LocationType>(LocationType::TYPE);

std::optional<LocationType> ArmorAddonOverrideService::checkLocationType(const std::unordered_set<std::string>& keywords,
                                                                         const WeatherFlags& weather_flags,
                                                                         const GameDayPart& day_part,
                                                                         RE::Actor* target) {
    // target must be loaded, and assigned
    if (actorOutfitAssignments.count(target) == 0 || !target || !target->Is3DLoaded())
        return {};

    RE::TESObjectCELL* cell = target->GetParentCell();
    bool inInterior = false;

    if (cell) {
        inInterior = cell->IsInteriorCell();

        // Action based location
        CHECK_LOCATION(Mounting, target->IsOnMount());
        CHECK_LOCATION(Swimming, target->AsActorState()->IsSwimming());
        CHECK_LOCATION(Sleeping, REUtilities::IsActorSleeping(target));
        CHECK_LOCATION(InWater, target->IsInWater());
        CHECK_LOCATION(Combat, target->IsInCombat());
    } else return LocationType::World;

    //Specific locations
    CHECK_LOCATION(PlayerHome, keywords.count("LocTypePlayerHouse"));
    CHECK_LOCATION(Castle, keywords.count("LocTypeCastle") || keywords.count("LocTypeMilitaryFort"));
    CHECK_LOCATION(Temple, keywords.count("LocTypeTemple"));
    CHECK_LOCATION(GuildHall, keywords.count("LocTypeGuild"));
    CHECK_LOCATION(Jail, keywords.count("LocTypeJail"));
    CHECK_LOCATION(Farm, keywords.count("LocTypeFarm") || keywords.count("LocTypeLumberMill"));
    CHECK_LOCATION(Military, keywords.count("LocTypeMilitaryCamp") || keywords.count("LocTypeBarracks"));
    CHECK_LOCATION(Inn, keywords.count("LocTypeInn"));
    CHECK_LOCATION(Store, keywords.count("LocTypeStore"));
    CHECK_LOCATION(Dungeon, keywords.count("LocTypeDungeon"));

    // Generic Locations
    CHECK_LOCATION(CityInterior, keywords.count("LocTypeCity") && inInterior);
    CHECK_LOCATION(CitySnow, keywords.count("LocTypeCity") && weather_flags.snowy);
    CHECK_LOCATION(CityRain, keywords.count("LocTypeCity") && weather_flags.rainy);
    CHECK_LOCATION(CityNight, keywords.count("LocTypeCity") && day_part == GameDayPart::Night);
    CHECK_LOCATION(City, keywords.count("LocTypeCity"));

    // A city is considered a town, so it will use the town outfit unless a city one is selected.
    CHECK_LOCATION(TownInterior, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && inInterior);
    CHECK_LOCATION(TownSnow, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && weather_flags.snowy);
    CHECK_LOCATION(TownRain, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && weather_flags.rainy);
    CHECK_LOCATION(TownNight, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && day_part == GameDayPart::Night);
    CHECK_LOCATION(Town, keywords.count("LocTypeTown") + keywords.count("LocTypeCity"));

    CHECK_LOCATION(WorldInterior, inInterior);
    CHECK_LOCATION(WorldSnow, weather_flags.snowy);
    CHECK_LOCATION(WorldRain, weather_flags.rainy);
    CHECK_LOCATION(WorldNight, day_part == GameDayPart::Night);
    CHECK_LOCATION(World, true);

    return {};
}

bool ArmorAddonOverrideService::shouldOverride(RE::Actor* target) const noexcept {
    if (!enabled)
        return false;
    if (actorOutfitAssignments.count(target) == 0)
        return false;
    if (actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName)
        return false;
    return true;
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out, bool favoritesOnly) const {
    out.clear();
    auto& list = outfits;
    out.reserve(list.size());
    for (auto it = list.cbegin(); it != list.cend(); ++it)
        if (!favoritesOnly || it->second.m_favorited)
            out.push_back(it->second.m_name);
}

void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
    enabled = flag;
}

InventoryManagementMode ArmorAddonOverrideService::getPlayerInventoryManagementMode() const noexcept {
    return playerInventoryManagementMode;
}

void ArmorAddonOverrideService::setPlayerInventoryManagementMode(InventoryManagementMode mode) noexcept {
    playerInventoryManagementMode = mode;
}

InventoryManagementMode ArmorAddonOverrideService::getNPCInventoryManagementMode() const noexcept {
    return npcInventoryManagementMode;
}

void ArmorAddonOverrideService::setNPCInventoryManagementMode(InventoryManagementMode mode) noexcept {
    npcInventoryManagementMode = mode;
}

proto::OutfitSystem ArmorAddonOverrideService::save() {
    proto::OutfitSystem out;
    out.set_enabled(enabled);
    out.set_player_inventory_management_mode(static_cast<uint32_t>(playerInventoryManagementMode));
    out.set_npc_inventory_management_mode(static_cast<uint32_t>(npcInventoryManagementMode));
    for (const auto& actorAssn : actorOutfitAssignments) {
        // Store a reference to the actor
        RE::Actor* actor = actorAssn.first;

        std::string actorFormString = Forms::GetFormString(actor);

        proto::ActorOutfitAssignment assnOut;
        assnOut.set_current_outfit_name(actorAssn.second.currentOutfitName.data(),
                                        actorAssn.second.currentOutfitName.size());
        for (const auto& locationBasedOutfit : actorAssn.second.locationOutfits) {
            assnOut.mutable_location_based_outfits()
                ->insert({
                    static_cast<std::uint32_t>(locationBasedOutfit.first),
                    std::string(locationBasedOutfit.second.data(), locationBasedOutfit.second.size())
                });
        }
        out.mutable_actor_outfit_assignments()->insert({actorFormString, assnOut});
    }
    for (const auto& outfit : outfits) {
        auto newOutfit = out.add_outfits();
        *newOutfit = outfit.second.save();
    }
    return out;
}
//
void ArmorAddonOverrideService::dump() const {
    LOG(info, "Dumping all state for ArmorAddonOverrideService...");
    LOG(info, "Enabled: %d", enabled);
    LOG(info, "We have %d outfits. Enumerating...", outfits.size());
    for (auto it = outfits.begin(); it != outfits.end(); ++it) {
        LOG(info, " - Key: %s", it->first.c_str());
        LOG(info, "    - Name: %s", it->second.m_name.c_str());
        LOG(info, "    - Armors:");
        auto& list = it->second.m_armors;
        for (auto jt = list.begin(); jt != list.end(); ++jt) {
            auto ptr = *jt;
            if (ptr) {
                LOG(info, "       - (TESObjectARMO*){} == [ARMO:{}]", (void*) ptr, ptr->formID);
            } else {
                LOG(info, "       - nullptr");
            }
        }
    }
    LOG(info, "All state has been dumped.");
}
