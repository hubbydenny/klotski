#pragma once
#include <string>
#include <vector>

namespace scripts {
    struct entry_t {
        std::string name;
        bool loaded{};
        std::string error;
    };
    std::string directory();
    void refresh();
    std::vector<entry_t> entries();
    bool load(const std::string& name);
    void unload(const std::string& name);
    void paint();
    void set_in_game(bool value);
    void shutdown();
}
