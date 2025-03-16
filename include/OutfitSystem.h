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
}
