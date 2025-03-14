//
// Created by Koukibyou on 3/13/2025.
//

#pragma once

#include <sstream>

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#ifndef FORM_H
#define FORM_H

#endif //FORM_H

namespace Forms {

    std::uint32_t GetBaseID(std::uint32_t formID);
    std::uint32_t GetBaseID(RE::TESForm* obj);
    std::uint32_t GetModIndex(std::uint32_t formID);
    std::uint32_t GetModIndex(RE::TESForm* obj);
    std::uint32_t GetModIndex(const char* name);

    // Load Order
    void ClearModList();
    void SavePluginsList(SKSE::SerializationInterface * intfc);
    void LoadPluginList(SKSE::SerializationInterface * intfc);

    std::uint32_t ResolveFormID(std::uint32_t formID);
    RE::TESForm* ResolveFormKey(std::uint64_t key);

    // Form Keys
    std::uint64_t GetNewKey(std::uint64_t key);
    std::uint64_t GetNewKey(std::uint64_t key, std::string modName);
    std::uint64_t GetFormKey(RE::TESForm* form);
    RE::TESForm* GetFormKey(std::uint64_t key);

    inline std::uint32_t GetKeyType(std::uint64_t key) { return (std::uint32_t)(key >> 32); }
    inline std::uint32_t GetKeyID(std::uint64_t key) { return (std::uint32_t)(key); }

    bool IsFormString(const std::string &str);
    const std::string GetFormString(RE::TESForm* obj);
    RE::TESForm* ParseFormString(const std::string &objString);

    int GameGetForm(int formId);
    bool IsValidObject(RE::TESForm* obj, std::uint64_t formId);

}