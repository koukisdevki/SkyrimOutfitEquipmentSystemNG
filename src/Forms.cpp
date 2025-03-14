//
// Created by Koukibyou on 3/13/2025.
// Reference from PapyrusUtils
//

#include "Forms.h"

#include <unordered_map>
#include <functional>

namespace Forms {

    static std::unordered_map<std::uint32_t, std::uint32_t> s_savedModIndexMap;

    void ClearModList() {
        s_savedModIndexMap.clear();
    }

    std::uint32_t ResolveModIndex(std::uint32_t modIndex) {
        auto it = s_savedModIndexMap.find(modIndex);
        if (it != s_savedModIndexMap.end()) {
            return it->second;
        }
        return 0xFF;
    }

    std::uint32_t GetBaseID(std::uint32_t formID) {
        if (formID == 0) return 0;
        return ((formID >> 24 == 0xFE) ? formID & 0x00000FFF : formID & 0x00FFFFFF);
    }

    std::uint32_t GetBaseID(RE::TESForm* obj) {
        if (!obj) return 0;
        return ((obj->GetFormID() >> 24 == 0xFE) ? obj->GetFormID() & 0x00000FFF : obj->GetFormID() & 0x00FFFFFF);
    }

    std::uint32_t GetModIndex(std::uint32_t formID) {
        if (formID == 0) return 0;
        std::uint32_t modID = formID >> 24;
        if (modID == 0xFE) {
            modID = formID >> 12;
        }
        return modID;
    }

    std::uint32_t GetModIndex(RE::TESForm* obj) {
        if (!obj || obj->GetFormID() == 0) return 0;

        std::uint32_t modID = obj->GetFormID() >> 24;
        if (modID == 0xFE) {
            modID = obj->GetFormID() >> 12;
        }
        return modID;
    }

    std::uint32_t GetModIndex(const char* name) {
        auto dataHandler = RE::TESDataHandler::GetSingleton();
        const auto modInfo = dataHandler->LookupModByName(name);
        if (modInfo) {
            return modInfo->IsLight() ?
                (0xFE000 | modInfo->smallFileCompileIndex) :
                modInfo->compileIndex;
        }
        return 0xFF;
    }

    std::uint32_t ResolveFormID(std::uint32_t formID) {
        if (formID == 0) return 0;
        std::uint32_t modID = formID >> 24;

        if (modID == 0xFF) {
            return formID; // FF
        }
        else if (modID == 0xFE) {
            modID = formID >> 12;
        }

        std::uint32_t loadedModID = ResolveModIndex(modID);
        if (loadedModID < 0xFF) {
            return (formID & 0x00FFFFFF) | (((std::uint32_t)loadedModID) << 24); // ESP/ESM
        }
        else if (loadedModID > 0xFF) {
            return (loadedModID << 12) | (formID & 0x00000FFF); // ESL
        }

        return 0;
    }

    RE::TESForm* ResolveFormKey(std::uint64_t key) {
        if (key < 1) return nullptr;
        std::uint32_t type = (std::uint32_t)(key >> 32);
        std::uint32_t id = ResolveFormID((std::uint32_t)(key));
        RE::TESForm* form = id <= 0 ? nullptr : RE::TESForm::LookupByID(id);
        return form ? (type == static_cast<std::uint32_t>(form->GetFormType()) ? form : nullptr) : nullptr;
    }

    bool IsValidObject(RE::TESForm* obj, std::uint64_t formId) {
//        LOG(info,std::format("IsValidObject(0x%X, 0x%X)", obj ? obj->GetFormID() : 0, formId));
        if (!obj || formId == 0) return false;
//        LOG(info,std::format("\tType: %d", static_cast<int>(obj->GetFormType()));
        if ((std::uint32_t)(formId >> 32) != 0 && static_cast<std::uint32_t>(obj->GetFormType()) != (std::uint32_t)(formId >> 32)) return false;
        else if ((formId & 0xFFFFFFFF) != 0 && (std::uint32_t)(formId & 0xFFFFFFFF) != obj->GetFormID()) return false;
        else return true;
    }

    std::uint64_t GetFormKey(RE::TESForm* form) {
        if (!form) return 0;
        std::uint64_t key = form->GetFormID();
        key |= ((std::uint64_t)form->GetFormType()) << 32;
        return key;
    }

    RE::TESForm* GetFormKey(std::uint64_t key) {
        if (key < 1) return nullptr;
        std::uint32_t type = (std::uint32_t)(key >> 32);
        std::uint32_t id = (std::uint32_t)(key);
        return id != 0 ? RE::TESForm::LookupByID(id) : nullptr;
    }

    std::uint64_t GetNewKey(std::uint64_t key) {
        if (key == 0) return 0;
        std::uint32_t type = (std::uint32_t)(key >> 32);
        std::uint32_t id = ResolveFormID((std::uint32_t)(key));
        return (((std::uint64_t)type) << 32) | (std::uint64_t)id;
    }

    std::uint64_t GetNewKey(std::uint64_t key, std::string modName) {
        // This is a placeholder since the original didn't implement this function
        return GetNewKey(key);
    }

    bool ends_with(const std::string &str, const std::string &suffix) {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    bool IsFormString(const std::string &str) {
        if (str.size() < 4 || str.find("|") == std::string::npos) return false;
        else return ends_with(str, ".esp") || ends_with(str, ".esm") || ends_with(str, ".esl") || ends_with(str, ".FF");
    }

    const std::string GetFormString(RE::TESForm* obj) {
        if (!obj) return "0";

        std::stringstream ss;
        std::uint32_t id = GetBaseID(obj);

        // Set hex base ID
        ss << "0x" << std::hex << id << "|";

        // Set Mod name
        std::uint32_t index = GetModIndex(obj);
        if (index < 0xFF) {
            // esp & esm objects
            auto dataHandler = RE::TESDataHandler::GetSingleton();
            const auto& modInfo = dataHandler->LookupLoadedModByIndex(index);
            if (modInfo) {
                ss << modInfo->fileName;
            }
        }
        else if (index > 0xFF) {
            // esl objects
            std::uint32_t light = index & 0x00FFF;
            auto dataHandler = RE::TESDataHandler::GetSingleton();
            const auto& modInfo = dataHandler->LookupLoadedLightModByIndex(light);
            if (modInfo) {
                ss << modInfo->fileName;
            }
        }
        else {
            // Temp objects - save form type as part of modname
            ss << static_cast<int>(obj->GetFormType()) << ".FF";
        }

        return ss.str();
    }

    RE::TESForm* ParseFormString(const std::string &objString) {
        if (!IsFormString(objString)) return nullptr;

        std::size_t pos = objString.find("|");
        std::string objID = objString.substr(0, pos);

        std::uint32_t obj = 0;
        if (objString.rfind("0x", 0) == 0) {
            obj = std::stoul(objID, nullptr, 16);
        }
        else {
            obj = std::stoi(objID);
        }

        std::string mod = objString.substr(pos + 1);

        if (ends_with(objString, ".FF")) {
            // Temp objects - check form type
            mod.resize((mod.length() - 3));
            std::uint8_t type = std::stoi(mod, nullptr, 16);
            obj = (((std::uint32_t)0xFF) << 24) | obj;
            RE::TESForm* objform = obj == 0 ? nullptr : RE::TESForm::LookupByID(obj);
            if (objform && static_cast<std::uint32_t>(objform->GetFormType()) == type) return objform;
            else {
                return nullptr;
            }
        }
        else {
            // esp & esm objects
            auto dataHandler = RE::TESDataHandler::GetSingleton();
            const auto modInfo = dataHandler->LookupModByName(mod.c_str());

            if (modInfo && modInfo->recordFlags.all(RE::TESFile::RecordFlag::kActive)) {
                if (modInfo->IsLight()) {
                    obj = (0xFE000 | modInfo->smallFileCompileIndex) << 12 | (obj & 0xFFF);
                } else {
                    obj = (modInfo->compileIndex << 24) | (obj & 0xFFFFFF);
                }
            }
            else {
                return nullptr; // Couldn't find mod.
            }
        }

        return obj == 0 ? nullptr : RE::TESForm::LookupByID(obj);
    }

    int GameGetForm(int formId) {
        if (formId == 0) return formId;
        auto form = RE::TESForm::LookupByID(formId);
        return form ? form->GetFormID() : 0;
    }
}
