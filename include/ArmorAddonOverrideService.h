#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "Utility.h"
#include "cobb/strings.h"
#include "outfit.pb.h"

namespace RE {
    class ActorWeightModel;

    namespace BIPED_OBJECTS_META {
        inline constexpr std::uint32_t kFirstSlot = RE::BIPED_OBJECTS::kHead;
        inline constexpr std::uint32_t kNumSlots = RE::BIPED_OBJECTS::kEditorTotal;
    }// namespace BIPED_OBJECTS_META
}// namespace RE

namespace RE {
    class TESObjectARMO;
}

enum class LocationType : std::uint32_t {
    // pairs of 100
    // Generic Locations
    World = 0,
    WorldNight = 100,
    WorldSnow = 200,
    WorldRain = 300,
    WorldInterior = 400,

    Town = 500,
    TownNight = 600,
    TownSnow = 700,
    TownRain = 800,
    TownInterior = 900,

    City = 1000,
    CityNight = 1100,
    CitySnow = 1200,
    CityRain = 1300,
    CityInterior = 1400,

    // While on an action
    Combat = 1500,
    InWater = 1600,
    Sleeping = 1700,
    Swimming = 1800,
    Mounting = 1900,

    // Specific Locations
    Dungeon = 5500,
    PlayerHome = 5600,
    Inn = 5700,
    Store = 5800,
    GuildHall = 5900,
    Castle = 6000,
    Temple = 6100,
    Farm = 6200,
    Jail = 6300,
    Military = 6400,
};

struct WeatherFlags {
    bool snowy = false;
    bool rainy = false;
};

struct Outfit {
    Outfit(const proto::Outfit& proto, const SKSE::SerializationInterface* intfc);
    Outfit(const char* n) : m_name(n), m_favorited(false){};
    Outfit(const Outfit& other) = default;
    Outfit(const char* n, const Outfit& other) : m_name(n), m_favorited(false) {
        m_armors = other.m_armors;
    }
    std::string m_name;// can't be const; prevents assigning to Outfit vars
    std::unordered_set<RE::TESObjectARMO*> m_armors;
    bool m_favorited;

    bool conflictsWith(RE::TESObjectARMO*) const;
    bool hasShield() const;
    std::unordered_set<RE::TESObjectARMO*> computeDisplaySet(const std::unordered_set<RE::TESObjectARMO*>& equippedSet);

    proto::Outfit save() const;// can throw ArmorAddonOverrideService::save_error

    bool operator==(const Outfit& rhs) const
    {
        // Compare all relevant fields
        return m_name == rhs.m_name &&
               m_armors == rhs.m_armors &&
               m_favorited == rhs.m_favorited;
    }
};
const constexpr char* g_noOutfitName = "";
static Outfit g_noOutfit(g_noOutfitName);// can't be const; prevents us from assigning it to Outfit&s

class ArmorAddonOverrideService {
public:
    ArmorAddonOverrideService(){};
    ArmorAddonOverrideService(const proto::OutfitSystem& data, const SKSE::SerializationInterface* intfc);// can throw load_error
    typedef Outfit Outfit;
    static constexpr std::uint32_t signature = 'AAOS';
    enum {
        kSaveVersionV1 = 1,
    };
    //
    static constexpr std::uint32_t ce_outfitNameMaxLength = 256;// SKSE caps serialized std::strings and const char*s to 256 bytes.
    //
    static void _validateNameOrThrow(const char* outfitName);
    //
    struct bad_name: public std::runtime_error {
        explicit bad_name(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct load_error: public std::runtime_error {
        explicit load_error(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct name_conflict: public std::runtime_error {
        explicit name_conflict(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct save_error: public std::runtime_error {
        explicit save_error(const std::string& what_arg) : runtime_error(what_arg){};
    };
    //
private:
    struct OutfitReferenceByName: public cobb::istring {
        OutfitReferenceByName(const value_type* s) : cobb::istring(s){};
        OutfitReferenceByName(const basic_string& other) : cobb::istring(other){};
        //
        operator Outfit&() {
            return ArmorAddonOverrideService::GetInstance().outfits.at(*this);
        }
    };

public:
    struct ActorOutfitAssignments {
        cobb::istring currentOutfitName = g_noOutfitName;
        std::map<LocationType, cobb::istring> locationOutfits;
    };
    bool enabled = true;
    std::map<cobb::istring, Outfit> outfits;
    std::map<RE::Actor*, ActorOutfitAssignments> actorOutfitAssignments;

    static ArmorAddonOverrideService& GetInstance() {
        static ArmorAddonOverrideService instance;
        return instance;
    };
    //
    Outfit& getOutfit(const char* name);        // throws std::out_of_range if not found
    Outfit& getOrCreateOutfit(const char* name);// can throw bad_name
    //
    void addOutfit(const char* name);                                        // can throw bad_name
    void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors);// can throw bad_name
    Outfit& currentOutfit(RE::Actor* target);
    bool hasOutfit(const char* name) const;
    void deleteOutfit(const char* name);
    void setFavorite(const char* name, bool favorite);
    void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false);// can throw bad_name if (createIfMissing)
    void renameOutfit(const char* oldName, const char* newName);                                                                                     // throws name_conflict if the new name is already taken; can throw bad_name; throws std::out_of_range if the oldName doesn't exist
    void setOutfit(const char* name, RE::Actor* target);
    void addActor(RE::Actor* target);
    void removeActor(RE::Actor* target);
    std::unordered_set<RE::Actor*> listActors();
    //
    void setOutfitUsingLocation(LocationType location, RE::Actor* target);
    void setLocationOutfit(LocationType location, const char* name, RE::Actor* target);
    void unsetLocationOutfit(LocationType location, RE::Actor* target);
    std::optional<cobb::istring> getLocationOutfit(LocationType location, RE::Actor* target);
    std::optional<LocationType> checkLocationType(const std::unordered_set<std::string>& keywords, const WeatherFlags& weather_flags, const GameDayPart& day_part, RE::Actor* target);
    //
    bool shouldOverride(RE::Actor* target) const noexcept;
    void getOutfitNames(std::vector<std::string>& out, bool favoritesOnly = false) const;
    void setEnabled(bool) noexcept;
    //
    proto::OutfitSystem save();// can throw save_error
    //
    void dump() const;
};