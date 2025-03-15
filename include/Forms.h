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
    bool IsFormString(const std::string &str);
    std::string GetFormString(RE::TESForm* obj);
    RE::TESForm* ParseFormString(const std::string &objString);
}