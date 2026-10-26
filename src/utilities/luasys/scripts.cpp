#include "scripts.hpp"
#include "lua/lua.hpp"
#include "../imgui/imgui.h"
#include "../console/console.hpp"
#include "../../features/config/config.hpp"
#include "../../includes.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mutex>

namespace {
    struct script_t {
        scripts::entry_t info;
        lua_State* state{};
    };
    std::vector<script_t> registry;
    std::mutex mutex;
    bool painting = false;
    bool in_game = false;

    unsigned draw_count = 0;

    void instruction_limit(lua_State* state, lua_Debug*) {
        luaL_error(state, "script instruction limit exceeded");
    }

    void budget(lua_State* state) { lua_sethook(state, instruction_limit, LUA_MASKCOUNT, 100000); }
    int log(lua_State* state) {
        console::log("[lua] %s\n", luaL_checkstring(state, 1));
        return 0;
    }
    int screen_size(lua_State* state) {
        lua_pushnumber(state, ImGui::GetIO().DisplaySize.x);
        lua_pushnumber(state, ImGui::GetIO().DisplaySize.y);
        return 2;
    }

    int time(lua_State* state) { 
        lua_pushnumber(state, ImGui::GetTime());
        return 1; 
    }

    int frame_time(lua_State* state) {
        lua_pushnumber(state,ImGui::GetIO().DeltaTime);
        return 1;
    }

    int is_in_game(lua_State* state) {
        lua_pushboolean(state,interfaces::engine->is_in_game()); 
        return 1; 
    }

    int connected(lua_State* state) {
        lua_pushboolean(state, interfaces::engine->is_connected());
        return 1;
    }
    int executecmd(lua_State* state, const auto* command) {
        lua_pushboolean(state, interfaces::engine->execute_cmd(command));
        return 1;
    }
    auto getlvlname(lua_State* state) { // should i consh char*?
        lua_pushstring(state, interfaces::engine->get_level_name());
        return 1;
    }
    auto getlvlname_short(lua_State* state) {
        lua_pushstring(state, interfaces::engine->get_level_name_short());
        return 1;
    }
    // will use ai to make types
//    auto vec3_t(lua_State* state) {
  //      lua_pushnumber(state, );
    // }
   // int visible(lua_State* state) {
     //   lua_pushboolean(state, is_visible());
     // return 1;
    //}


    float number(lua_State* state, int index) {
        const auto value = luaL_checknumber(state,index);
        if (!std::isfinite(value) || std::fabs(value) > 100000.f) luaL_argerror(state,index,"invalid coordinate");
        return static_cast<float>(value);
    }
    ImU32 color(lua_State* state, int start) {
        int rgba[4];
        for (int i=0;i<4;++i) rgba[i] = static_cast<int>(std::clamp<lua_Integer>(luaL_optinteger(state,start+i,255),0,255));
        return IM_COL32(rgba[0],rgba[1],rgba[2],rgba[3]);
    }
    void allow_drawing(lua_State* state) {
        if (!painting) luaL_error(state,"render is only available in on_paint");
        if (++draw_count > 2048) luaL_error(state,"draw limit exceeded");
    }
    int text(lua_State* state) {
        allow_drawing(state);
        const float x = number(state,1), y = number(state,2);
        size_t length{};
        const auto value = luaL_checklstring(state,3,&length);
        if (length > 4096) return luaL_argerror(state,3,"text is too long");
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(x,y),color(state,4),value,value+length);
        return 0;
    }
    int line(lua_State* state) {
        allow_drawing(state);
        const ImVec2 start(number(state,1),number(state,2));
        const ImVec2 end(number(state,3),number(state,4));
        const float thickness = static_cast<float>(luaL_optnumber(state,9,1.f));
        if (!std::isfinite(thickness) || thickness <= 0.f || thickness > 16.f) return luaL_argerror(state,9,"thickness must be in (0, 16]");
        ImGui::GetBackgroundDrawList()->AddLine(start,end,color(state,5),thickness);
        return 0;
    }
    int rect(lua_State* state) {
        allow_drawing(state);
        const float x=number(state,1), y=number(state,2), width=number(state,3), height=number(state,4);
        if (width<0.f || height<0.f) return luaL_error(state,"rectangle size must be non-negative");
        const auto tint = color(state,5);
        const bool filled = lua_isnoneornil(state,9) || lua_toboolean(state,9);
        if (filled) ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x,y),ImVec2(x+width,y+height),tint);
        else ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x,y),ImVec2(x+width,y+height),tint);
        return 0;
    }
    int circle(lua_State* state) {
        allow_drawing(state);
        const float x=number(state,1), y=number(state,2), radius=number(state,3);
        if (radius<0.f || radius>4096.f) return luaL_argerror(state,3,"radius must be in [0, 4096]");
        const auto tint = color(state,4);
        if (lua_toboolean(state,8)) ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x,y),radius,tint,48);
        else ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x,y),radius,tint,48);
        return 0;
    }
    int text_size(lua_State* state) {
        size_t length{};
        const auto text=luaL_checklstring(state,1,&length);
        if (length>4096) return luaL_argerror(state,1,"text is too long");
        const auto size = ImGui::CalcTextSize(text,text+length);
        lua_pushnumber(state,size.x); lua_pushnumber(state,size.y);
        return 2;
    }

    void api(lua_State* state) {
        luaL_requiref(state, "_G", luaopen_base, 1); lua_pop(state,1);
        luaL_requiref(state, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(state,1);
        luaL_requiref(state, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(state,1);
        luaL_requiref(state, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(state,1);
        for (auto name : {"dofile","loadfile","pcall","xpcall"}) { lua_pushnil(state); lua_setglobal(state,name); }
        const luaL_Reg client[] = 
        {{"log",log},
        {"screen_size",screen_size},
        {"time",time},
        {"frame_time",frame_time},
        {"is_in_game",is_in_game},
        {"is_connected", connected},
        {nullptr,nullptr}};
        luaL_newlib(state,client); lua_setglobal(state,"client");

        const luaL_Reg render[] = 
        {{"text",text},
        {"line",line},
        {"rect",rect},
        {"circle",circle},
        {"text_size",text_size},
        {nullptr,nullptr}};
        luaL_newlib(state,render); lua_setglobal(state,"render");
    }

    void close(script_t& script) {
        if (script.state) lua_close(script.state);
        script.state = nullptr;
        script.info.loaded = false;
    }
    void error(script_t& script) {
        const auto message = lua_tostring(script.state,-1);
        script.info.error = message ? message : "unknown Lua error";
        console::log("[lua] %s: %s\n",script.info.name.c_str(),script.info.error.c_str());
        close(script);
    }
}

std::string scripts::directory() {
    auto path = std::filesystem::path(config::get_directory()) / "scripts";
    std::error_code ec;
    std::filesystem::create_directories(path,ec);
    return path.string();
}

void scripts::refresh() {
    std::lock_guard lock(mutex);
    std::error_code ec;
    const std::filesystem::path root(directory());
    for (auto& script : registry) {
        if (!std::filesystem::is_regular_file(root / script.info.name,ec)) close(script);
        ec.clear();
    }
    std::erase_if(registry,[&](const auto& script) { return !std::filesystem::is_regular_file(root/script.info.name,ec); });
    for (std::filesystem::directory_iterator it(root,ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec) || it->path().extension() != ".lua") continue;
        const auto name = it->path().filename().string();
        if (std::none_of(registry.begin(),registry.end(),[&](const auto& script){ return script.info.name == name; }))
            registry.push_back({{name,false,{}},nullptr});
    }
    std::sort(registry.begin(),registry.end(),[](const auto& a,const auto& b){return a.info.name < b.info.name;});
}

std::vector<scripts::entry_t> scripts::entries() {
    std::lock_guard lock(mutex);
    std::vector<entry_t> result;
    for (const auto& script : registry) result.push_back(script.info);
    return result;
}

bool scripts::load(const std::string& name) {
    std::lock_guard lock(mutex);
    auto it = std::find_if(registry.begin(),registry.end(),[&](const auto& script){return script.info.name == name;});
    if (it == registry.end()) return false;
    close(*it);
    it->info.error.clear();
    it->state = luaL_newstate();
    if (!it->state) { it->info.error = "unable to create Lua state"; return false; }
    api(it->state);
    budget(it->state);
    const auto path = std::filesystem::path(directory()) / name;
    if (luaL_loadfilex(it->state,path.string().c_str(),"t") != LUA_OK || lua_pcall(it->state,0,0,0) != LUA_OK) { error(*it); return false; }
    it->info.loaded = true;
    return true;
}

void scripts::unload(const std::string& name) {
    std::lock_guard lock(mutex);
    for (auto& script : registry) if (script.info.name == name) { close(script); script.info.error.clear(); }
}

void scripts::paint() {
    std::lock_guard lock(mutex);
    for (auto& script : registry) {
        if (!script.state) continue;
        lua_getglobal(script.state,"on_paint");
        if (!lua_isfunction(script.state,-1)) { lua_pop(script.state,1); continue; }
        budget(script.state);
        draw_count = 0;
        painting = true;
        const auto result = lua_pcall(script.state,0,0,0);
        painting = false;
        if (result != LUA_OK) error(script);
    }
}

void scripts::shutdown() {
    std::lock_guard lock(mutex);
    for (auto& script : registry) close(script);
    registry.clear();
}

void scripts::set_in_game(bool value) {
    std::lock_guard lock(mutex);
    in_game = value;
}
