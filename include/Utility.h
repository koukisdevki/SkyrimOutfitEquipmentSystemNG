#pragma once

#include <string>

std::string GetRuntimeName();

const std::string& GetRuntimeDirectory();

class Settings {
public:
    Settings();
    ~Settings();
    INIReader reader;
    static INIReader* Instance();
};

class LogExit {
public:
    std::string_view m_string;
    LogExit(std::string_view name) : m_string(name) {
        LOG(trace, "Enter {}", m_string);
    };
    ~LogExit() {
        LOG(trace, "Exit {}", m_string);
    };
};

enum class GameDayPart : uint8_t {
    Day = 0,
    Night = 1,
};

struct GameTime {
    int hour;    // 0-23
    int minute;  // 0-59

    // Equality operator
    bool operator==(const GameTime& other) const {
        return hour == other.hour && minute == other.minute;
    }
};

namespace REUtilities {
    bool IsActorSleeping(RE::Actor* actor);
    GameTime CurrentGameHour();
    GameDayPart CurrentGameDayPart();
}
