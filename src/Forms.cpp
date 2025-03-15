#include "Forms.h"

namespace Forms {

    bool IsFormString(const std::string &str) {
        if (str.size() < 4 || str.find("|") == std::string::npos) return false;

        return str.ends_with(".esp") || str.ends_with(".esm") || str.ends_with(".esl") || str.ends_with(".FF");
    }

    uint32_t GetBaseID(uint32_t formID) {
        if (formID == 0) return 0;
        return ((formID >> 24 == 0xFE) ? formID & 0x00000FFF : formID & 0x00FFFFFF);
    }

    uint32_t GetBaseID(RE::TESForm* obj) {
        if (!obj) return 0;
        return ((obj->formID >> 24 == 0xFE) ? obj->formID & 0x00000FFF : obj->formID & 0x00FFFFFF);
    }

    uint32_t GetModIndex(RE::TESForm* obj) {
        if (obj->formID == 0) return 0;

        uint32_t modID = obj->formID >> 24;
        if (modID == 0xFE) {
            modID = obj->formID >> 12;
        }
        return modID;
    }

    std::string GetFormString(RE::TESForm *obj) {
        if (!obj) {
            LOG(critical, "GetFormString called with null object");
            return "0";
        }

        std::stringstream ss;
        uint32_t id = GetBaseID(obj);

        // Set hex base ID
        ss << "0x" << std::hex << id << "|";

        //LOG( "GetFormString processing form 0x{:X} ({})", obj->GetFormID(), obj->GetName());

        RE::TESDataHandler *Data = RE::TESDataHandler::GetSingleton();
        if (!Data) {
            LOG(critical, "Failed to get TESDataHandler singleton");
            return "0";
        }

        // Set Mod name
        uint32_t index = GetModIndex(obj);
        //LOG( "Form mod index is 0x{:X}", index);

        if (index < 0xFF) {
            // esp & esm objects
            RE::TESFile** loadedMods = Data->GetLoadedMods();
            if (!loadedMods) {
                LOG(critical, "Failed to get loaded mods array");
                return "0";
            }

            RE::TESFile *modInfo = loadedMods[index];
            if (!modInfo) {
                LOG(critical, "Null mod info for index {}", index);
                return "0";
            }

            ss << modInfo->GetFilename();
            //LOG( "Form belongs to ESP/ESM file: {}", modInfo->GetFilename());
        }
        else if (index > 0xFF) {
            // esl objects
            RE::TESFile** loadedCCMods = Data->GetLoadedLightMods();
            if (!loadedCCMods) {
                LOG(critical, "Failed to get loaded light mods array");
                return "0";
            }

            uint32_t light = index & 0x00FFF;
            //LOG( "Light mod index is 0x{:X}", light);

            RE::TESFile *modInfo = loadedCCMods[light];
            if (!modInfo) {
                LOG(critical, "Null light mod info for light index {}", light);
                return "0";
            }

            ss << modInfo->GetFilename();
            //LOG( "Form belongs to ESL file: {}", modInfo->GetFilename());
        }
        else {
            // Temp objects - save form type as part of modname
            uint8_t formType = static_cast<uint8_t>(obj->GetFormType());
            ss << static_cast<int>(formType) << ".FF";
            //LOG( "Form is a temporary object with form type {}", formType);
        }

        const std::string out = ss.str();
        //LOG( "GetFormString result: {}", out);
        return out;
    }

    RE::TESForm* ParseFormString(const std::string &objString) {
        //LOG( "ParseFormString called with: {}", objString);

        if (!IsFormString(objString)) {
            LOG(critical, "Invalid form string format: {}", objString);
            return nullptr;
        }

        std::size_t pos = objString.find('|');
        std::string objID = objString.substr(0, pos);
        //LOG( "Extracted ID part: {}", objID);

        uint32_t baseId = 0;
        try {
            if (objID.rfind("0x", 0) == 0) {
                baseId = std::stoul(objID, nullptr, 16);
            } else {
                baseId = std::stoi(objID);
            }
            //LOG( "Parsed base ID: 0x{:X}", baseId);
        } catch (const std::exception& e) {
            LOG(critical, "Failed to parse form ID '{}': {}", objID, e.what());
            return nullptr;
        }

        std::string mod = objString.substr(pos + 1);
        //LOG( "Extracted mod part: {}", mod);

        if (objString.ends_with(".FF")) {
            //LOG( "Processing temporary form");
            // Temp objects - check form type
            mod.resize((mod.length() - 3));

            uint8_t type;
            try {
                type = std::stoi(mod, nullptr, 10);
                //LOG( "Parsed form type: {}", type);
            } catch (const std::exception& e) {
                LOG(critical, "Failed to parse form type '{}': {}", mod, e.what());
                return nullptr;
            }

            uint32_t formId = (static_cast<uint32_t>(0xFF) << 24) | baseId;
            //LOG( "Constructed temporary form ID: 0x{:X}", formId);

            RE::TESForm* objform = formId == 0 ? nullptr : RE::TESForm::LookupByID(formId);
            if (!objform) {
                LOG(critical, "Form not found with ID 0x{:X}", formId);
                return nullptr;
            }

            uint8_t actualType = static_cast<uint8_t>(objform->GetFormType());
            //LOG( "Form found, expected type: {}, actual type: {}", type, actualType);

            if (actualType == type) {
                //LOG( "Form type matches, returning form 0x{:X}", formId);
                return objform;
            } else {
                LOG(critical, "Form type mismatch, expected: {}, got: {}", type, actualType);
                return nullptr;
            }
        }

        //LOG( "Looking up mod: {}", mod);
        RE::TESDataHandler* dhand = RE::TESDataHandler::GetSingleton();
        if (!dhand) {
            LOG(critical, "Failed to get TESDataHandler singleton");
            return nullptr;
        }

        const RE::TESFile* modInfo = dhand->LookupModByName(mod);
        if (!modInfo) {
            LOG(critical, "Mod not found: {}", mod);
            return nullptr;
        }

        uint32_t formId = 0;
        // Construct form ID based on whether it's a light mod or not
        if (modInfo->IsLight()) {
            uint16_t smallFileIndex = modInfo->GetSmallFileCompileIndex();
            //LOG( "Light mod (ESL), small file index: 0x{:X}", smallFileIndex);
            formId = 0xFE000000 | (static_cast<uint32_t>(smallFileIndex) << 12) | (baseId & 0xFFF);
        } else {
            uint8_t compileIndex = modInfo->GetCompileIndex();
            //LOG( "Regular mod (ESP/ESM), compile index: 0x{:X}", compileIndex);
            formId = (static_cast<uint32_t>(compileIndex) << 24) | baseId;
        }

        //LOG(info, "Constructed full form ID: 0x{:X}", formId);

        RE::TESForm* form = formId == 0 ? nullptr : RE::TESForm::LookupByID(formId);
        if (!form) {
            LOG(critical, "Form not found with ID 0x{:X}", formId);
            return nullptr;
        }

        //LOG(info, "Form found: 0x{:X} ({})", formId, form->GetName());
        return form;
    }
}