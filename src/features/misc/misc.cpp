#include "misc.hpp"

#include "../config/config.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../utilities/renderer/renderer.hpp"
#include "../../utilities/imgui/imgui.h"
import players_class;
#include "../../source2-sdk/interfaces/i_engine_client.hpp"
import class_user_cmd;
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cctype>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <d3d11.h>
#include <wrl/client.h>
#include <Windows.h>
#include "../menu/menu.hpp"
#include "../menu/overlay.hpp"
#include "../movement/movement.hpp"

static std::atomic<unsigned> practice_pending{ 0 };
static std::atomic<bool> practice_saved{ false };

void misc::practice_request(bool load)
{
	practice_pending.fetch_or(load ? 2u : 1u, std::memory_order_relaxed);
}

static void practice_input() {
	static bool previous_save = false;
	static bool previous_load = false;
	const int save_key = config::context.practice_save_key;
	const int load_key = config::context.practice_load_key;
	const bool save_down = save_key > 0 && save_key < 256 && (GetAsyncKeyState(save_key) & 0x8000);
	const bool load_down = load_key > 0 && load_key < 256 && (GetAsyncKeyState(load_key) & 0x8000);
	const bool in_game = interfaces::engine && interfaces::engine->is_in_game();
	if (!in_game) {
		practice_pending.store(4u, std::memory_order_relaxed);
		practice_saved.store(false, std::memory_order_relaxed);
	}
	else if (!config::context.practice)
		practice_pending.fetch_and(4u, std::memory_order_relaxed);
	else if (!menu::open)
	{
		if (save_down && !previous_save) misc::practice_request(false);
		if (load_down && !previous_load) misc::practice_request(true);
	}
	previous_save = save_down;
	previous_load = load_down;
}

void misc::practice_move(user_cmd_t* cmd) {
	static bool saved = false;
	static vec3_t origin{};
	static vec3_t angles{};
	auto local = sdk::local_player;
	if (!interfaces::engine || !interfaces::engine->is_in_game())
	{
		saved = false;
		practice_saved.store(false, std::memory_order_relaxed);
		practice_pending.store(0u, std::memory_order_relaxed);
		return;
	}
	if (!cmd || !config::context.practice) return;
	if (!local || !local->is_alive())
	{
		practice_pending.fetch_and(4u, std::memory_order_relaxed);
		return;
	}
	const auto actions = practice_pending.exchange(0u, std::memory_order_relaxed);
	if (actions & 4u) {
		saved = false;
		practice_saved.store(false, std::memory_order_relaxed);
	}
	if (actions & 1u)
	{
		auto view = cmd->get_view_angles();
		vec3_t position{};


		if (!local->has_flag(fl_onground))
			utilities::chat_print("you have to be on ground while saving checkpoint");
		else if (view && local->try_abs_origin(position))
		{
			origin = position;
			angles = view->angles;
			saved = true;
			practice_saved.store(true, std::memory_order_relaxed);
			utilities::chat_print("checkpoint saved");
		}
		else utilities::chat_print("unable to save checkpoint");
	}

	if (!(actions & 2u)) return;
	if (!saved)
	{
		utilities::chat_print("no checkpoint saved");
		return;
	}
	auto cheats = interfaces::cvar ? interfaces::cvar->FindVar("sv_cheats") : nullptr;
	if (!cheats || !cheats->get_bool()) {
		utilities::chat_print("unable due to sv_cheats 0");
		return;
	}
	char command[256]{};
	std::snprintf(command, sizeof(command), "setpos %.6f %.6f %.6f; setang %.6f %.6f %.6f", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z);
	if (!interfaces::engine->execute_cmd(command)) utilities::chat_print("unable send cmd");
}

static void draw_watermark() {
	if (!config::context.watermark) return;

	if (menu::font) ImGui::PushFont(menu::font);
	const auto& io = ImGui::GetIO();
    SYSTEMTIME time{};
    GetLocalTime(&time);
    char label[32]{};
    std::snprintf(label,sizeof(label),"klotski | %02u:%02u:%02u",
        static_cast<unsigned>(time.wHour),static_cast<unsigned>(time.wMinute),static_cast<unsigned>(time.wSecond));
    const auto text_size = ImGui::CalcTextSize(label);
    const float width = text_size.x+15.f;
    const float height = text_size.y+12.f;
    if (io.DisplaySize.x < width || io.DisplaySize.y < height) {
        if (menu::font) ImGui::PopFont();
        return;
    }
	const float right_margin = std::isfinite(config::context.watermark_x) ? (std::max)(0.f, config::context.watermark_x) : 12.f;
	const float top_margin = std::isfinite(config::context.watermark_y) ? config::context.watermark_y : 12.f;
	const float x = std::floor((std::max)(0.f, io.DisplaySize.x - width - right_margin));
	const float y = std::floor(std::clamp(top_margin, 0.f, (std::max)(0.f, io.DisplaySize.y - height)));
	auto draw = ImGui::GetForegroundDrawList();
	const auto& accent = config::context.menu_accent;
	const auto accent_color = ImGui::ColorConvertFloat4ToU32(ImVec4(accent[0], accent[1], accent[2], 1.f));
    menu::overlay::surface(draw,ImVec2(x,y),ImVec2(width,height),accent_color);
    draw->AddText(ImVec2(x+8.f,y+7.f),menu::overlay::text,label);
	if (menu::font) ImGui::PopFont();
}

struct velocity_sample_t {
	player_t* player = nullptr;
	float speed = 0.f;
	float time = 0.f;
	float takeoff_until = 0.f;
	int takeoff_velocity = 0;
	float last_velocity = 0.f;
	int color_index = 2;
	int last_tick = 0;
	std::chrono::steady_clock::time_point color_sample_time{};
	bool grounded = false;
	bool has_takeoff = false;
};
static velocity_sample_t velocity_sample{};
static std::mutex velocity_mutex;

void misc::update_velocity()
{
	std::lock_guard<std::mutex> guard(velocity_mutex);
	auto local = sdk::local_player;
	if (!local || !local->is_alive() || !interfaces::globals)
	{
		velocity_sample = {};
		return;
	}
	auto& sample = velocity_sample;
	const int tick = interfaces::globals->tick_count;
	const auto now = std::chrono::steady_clock::now();
	const float speed = vel2d(local->velocity());
	const int velocity = static_cast<int>(std::round(speed));
	const bool grounded = local->flags() & fl_onground;
	if (sample.player != local || tick < sample.last_tick || interfaces::globals->curtime < sample.time)
	{
		sample = {};
		sample.player = local;
		sample.grounded = grounded;
		sample.last_velocity = speed;
		sample.last_tick = tick;
		sample.color_sample_time = now;
	}
	sample.time = interfaces::globals->curtime;
	if (sample.grounded && !grounded)
	{
		sample.takeoff_velocity = velocity;
		sample.takeoff_until = sample.time + 2.f;
		sample.has_takeoff = true;
	}
	sample.speed = speed;
	sample.grounded = grounded;
	if (now - sample.color_sample_time >= std::chrono::milliseconds(10))
	{
		const float delta = speed - sample.last_velocity;
		sample.color_index = delta > 0.5f ? 4 : delta < -0.5f ? 3 : 2;
		sample.last_velocity = speed;
		sample.color_sample_time = now;
	}
	sample.last_tick = tick;
}

static void velocityind()
{
	if (!config::context.velocity || !interfaces::engine || !interfaces::engine->is_in_game()) return;
	velocity_sample_t sample; {
		std::lock_guard<std::mutex> guard(velocity_mutex);
		sample = velocity_sample;
	}
	if (!sample.player || sample.player != sdk::local_player) return;
	const int velocity = static_cast<int>(std::round(sample.speed));
	const float blend = (std::clamp)(sample.speed / 310.f, 0.f, 1.f);
	const int index = sample.color_index;
	float rgb[3]{};
	for (int i = 0; i < 3; i++)
		rgb[i] = config::context.velo_gradient ? config::context.velo_colors[0][i] + (config::context.velo_colors[1][i] - config::context.velo_colors[0][i]) * blend : config::context.velo_colors[index][i];
	const color_t color(static_cast<std::uint8_t>((std::clamp)(rgb[0], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[1], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((std::clamp)(rgb[2], 0.f, 1.f) * 255.f), static_cast<std::uint8_t>((config::context.velo_fade ? blend : 1.f) * 255.f));
	char buffer[64]{};
	if (config::context.velo_pre_speed && sample.has_takeoff && (!sample.grounded || sample.time < sample.takeoff_until))
		sprintf_s(buffer, sizeof(buffer), "%d (%d)", velocity, sample.takeoff_velocity);
	else sprintf_s(buffer, sizeof(buffer), "%d", velocity);
	const int size = (std::clamp)(static_cast<int>(std::round(config::context.velo_size)), 8, 48);
	auto font = renderer::velocity_fonts[size];
	if (!font) font = ImGui::GetFont();
	renderer::text_options_t options{};
	options.font = font;
	options.centered = true;
	options.drop_shadow = true;
	const float x = std::floor(static_cast<float>(sdk::screen_width) * 0.5f);
	const float y = std::floor((std::clamp)(static_cast<float>(sdk::screen_height) - config::context.velo_y, 0.f, (std::max)(0.f, static_cast<float>(sdk::screen_height - size))));
	renderer::text(x, y, static_cast<float>(size), color, buffer, options);
}

bool duck_held = false;
bool jump_held = false;

static auto cmdhelper(user_cmd_t* cmd) {
	duck_held = cmd->is_button_down(in_duck);
	jump_held = cmd->is_button_down(in_jump);
}
static void keystrokes() {
	static float highlight[6]{};
	if (!config::context.keystrokes) {
		std::fill(std::begin(highlight), std::end(highlight), 0.f);
		return;
	}
	const bool pressed[] = {
		ImGui::IsKeyDown(ImGuiKey_W), ImGui::IsKeyDown(ImGuiKey_A),
		ImGui::IsKeyDown(ImGuiKey_S), ImGui::IsKeyDown(ImGuiKey_D),
		config::context.show_crouch && (duck_held || ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)),
		config::context.show_in_jump && (jump_held || ImGui::IsKeyDown(ImGuiKey_Space))
	};
	const auto& io = ImGui::GetIO();
	const float blend = 1.f - std::exp(-24.f * (std::clamp)(io.DeltaTime, 0.f, .1f));
	for (int i = 0; i < 6; ++i) highlight[i] += ((pressed[i] ? 1.f : 0.f) - highlight[i]) * blend;
	if (!(config::context.show_in_menu && menu::open)
		&& std::none_of(std::begin(pressed), std::end(pressed), [](bool value) { return value; })) return;

	const float setting = std::isfinite(config::context.keystrokes_size) ? config::context.keystrokes_size : 16.f;
	const float cell = (std::clamp)(setting, 16.f, 48.f);
	constexpr float gap = 3.f;
	const float width = cell * 3.f + gap * 2.f;
	const bool bottom = config::context.show_crouch || config::context.show_in_jump;
	const float height = cell * 2.f + gap + (bottom ? cell * .7f + gap : 0.f);
	if (io.DisplaySize.x < width || io.DisplaySize.y < height) return;
	const auto position = config::context.keystroke_pos;
	const float x = std::floor((std::clamp)(std::isfinite(position.x) ? position.x : 0.f, 0.f, io.DisplaySize.x - width));
	const float y = std::floor((std::clamp)(std::isfinite(position.y) ? position.y : 0.f, 0.f, io.DisplaySize.y - height));
	auto draw = ImGui::GetForegroundDrawList();
	auto font = menu::font ? menu::font : ImGui::GetFont();
	const auto& accent = config::context.menu_accent;
	draw->AddRectFilled(ImVec2(x - 5.f, y - 5.f), ImVec2(x + width + 5.f, y + height + 5.f), IM_COL32(0, 0, 0, 120), 4.f);
	draw->AddRect(ImVec2(x - 4.5f, y - 4.5f), ImVec2(x + width + 4.5f, y + height + 4.5f), IM_COL32(62, 62, 70, 220), 4.f);
	const auto tile = [&](float left, float top, float tile_width, float tile_height, const char* label, int index) {
		const ImVec2 corner_min(left, top), corner_max(left + tile_width, top + tile_height);
		const float active = highlight[index];
		const auto fill = ImGui::ColorConvertFloat4ToU32(ImVec4(
			std::lerp(.075f, accent[0] * .72f, active), std::lerp(.075f, accent[1] * .72f, active),
			std::lerp(.085f, accent[2] * .72f, active), .98f));
		const auto border = ImGui::ColorConvertFloat4ToU32(ImVec4(
			std::lerp(.21f, accent[0], active), std::lerp(.21f, accent[1], active), std::lerp(.24f, accent[2], active), 1.f));
		draw->AddRectFilled(corner_min, corner_max, fill, 3.f);
		draw->AddRect(ImVec2(left + .5f, top + .5f), ImVec2(corner_max.x - .5f, corner_max.y - .5f), border, 3.f);
		const float text_size_px = (std::min)(font->FontSize, tile_height * .42f);
		const auto text_size = font->CalcTextSizeA(text_size_px, FLT_MAX, 0.f, label);
		draw->AddText(font, text_size_px, ImVec2(left + (tile_width - text_size.x) * .5f, top + (tile_height - text_size.y) * .5f),
			ImGui::ColorConvertFloat4ToU32(ImVec4(std::lerp(.65f, 1.f, active), std::lerp(.65f, 1.f, active), std::lerp(.70f, 1.f, active), 1.f)), label);
	};
	tile(x + cell + gap, y, cell, cell, "W", 0);
	tile(x, y + cell + gap, cell, cell, "A", 1);
	tile(x + cell + gap, y + cell + gap, cell, cell, "S", 2);
	tile(x + (cell + gap) * 2.f, y + cell + gap, cell, cell, "D", 3);
	const float bottom_y = y + (cell + gap) * 2.f;
	const bool both = config::context.show_crouch && config::context.show_in_jump;
	const float crouch_width = both ? cell : width;
	if (config::context.show_crouch) tile(x, bottom_y, crouch_width, cell * .7f, "CTRL", 4);
	if (config::context.show_in_jump) tile(x + (both ? cell + gap : 0.f), bottom_y, both ? cell * 2.f + gap : width, cell * .7f, "SPACE", 5);
}
static void indicators() {
	static float alpha[6]{};
	auto local = sdk::local_player;
	if (!config::context.movement_indicators || !interfaces::engine || !interfaces::engine->is_in_game()
		|| !local || !local->is_alive()) {
		std::fill(std::begin(alpha), std::end(alpha), 0.f);
		return;
	}
	const char* names[] = { "eb", "ej", "lj", "mj", "jb", "lb", "ab"};
	const int slots[] = { 0, 2, 3, 4, 5, 6 };
	const bool active[] = { config::context.edgebug, config::context.ej, config::context.longjump, config::context.mj, config::context.jumpbug, config::context.ladder_bug };
	const bool success[] = { movement::edgebug_active(), movement::ej_active(), movement::lj_active, movement::mj_active, movement::jumpbug_detected(), false };
	const auto& io = ImGui::GetIO();
	const int size = static_cast<int>((std::clamp)(config::context.indicator_size, 8.f, 48.f));
	auto font = renderer::velocity_fonts[size] ? renderer::velocity_fonts[size] : ImGui::GetFont();
	const float row_height = font->CalcTextSizeA(static_cast<float>(size), FLT_MAX, 0.f, "eb").y;
	constexpr float gap = 10.f;
	const bool preview = config::context.show_in_menu && menu::open;
	for (int i = 0; i < 6; ++i) {
		const bool visible = config::context.indicator_items[slots[i]] && (active[i] || preview);
		alpha[i] = visible ? 1.f : 0.f;
	}
	float total_width = 0.f;
	for (int i = 0; i < 6; ++i) {
		if (alpha[i] <= 0.f) continue;
		total_width += font->CalcTextSizeA(static_cast<float>(size), FLT_MAX, 0.f, names[i]).x * alpha[i];
	}
	int visible_count = 0;
	for (int i = 0; i < 6; ++i) if (alpha[i] > 0.f) ++visible_count;
	total_width += gap * (std::max)(0, visible_count - 1);
	const float y = (std::clamp)(io.DisplaySize.y - config::context.indicator_offset - row_height,
		0.f, (std::max)(0.f, io.DisplaySize.y - row_height));
	float x = (io.DisplaySize.x - total_width) * .5f;
	auto draw = ImGui::GetBackgroundDrawList();
	for (int i = 0; i < 6; ++i) {
		if (alpha[i] <= 0.f) continue;
		const auto& rgb = success[i] ? config::context.indicator_success_color : config::context.indicator_color;
		const float opacity = alpha[i] * (std::clamp)(rgb[3], 0.f, 1.f);
		const auto measured = font->CalcTextSizeA(static_cast<float>(size), FLT_MAX, 0.f, names[i]);
		const auto color = ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], opacity));
		const auto outline = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, opacity));
		draw->AddText(font, static_cast<float>(size), ImVec2(std::floor(x + 1.f), y + 1.f), outline, names[i]);
		draw->AddText(font, static_cast<float>(size), ImVec2(std::floor(x), y), color, names[i]);
		x += measured.x * alpha[i] + gap;
	}
};

//pasted
static ID3D11ShaderResourceView* spectator_avatar(std::uint64_t steam_id) {
    using Microsoft::WRL::ComPtr;
    struct avatar_entry {
        ComPtr<ID3D11ShaderResourceView> texture;
        ULONGLONG retry_after{};
    };
    static std::unordered_map<std::uint64_t, avatar_entry> entries;
    static ComPtr<ID3D11Device> cached_device;
    auto font_texture = reinterpret_cast<ID3D11ShaderResourceView*>(ImGui::GetIO().Fonts->TexID);
    if (!font_texture || !steam_id) return nullptr;
    ComPtr<ID3D11Device> device;
    font_texture->GetDevice(device.GetAddressOf());
    if (device.Get() != cached_device.Get()) { entries.clear(); cached_device = device; }
    auto& entry = entries[steam_id];
    if (entry.texture) return entry.texture.Get();
    const auto now = GetTickCount64();
    if (now < entry.retry_after) return nullptr;
    entry.retry_after = now + 1000;
    auto module = GetModuleHandleA("steam_api64.dll");
    if (!module) return nullptr;
    const auto friends_fn = reinterpret_cast<void* (*)()>(GetProcAddress(module, "SteamAPI_SteamFriends_v018"));
    const auto utils_fn = reinterpret_cast<void* (*)()>(GetProcAddress(module, "SteamAPI_SteamUtils_v010"));
    const auto avatar_fn = reinterpret_cast<int (*)(void*, std::uint64_t)>(GetProcAddress(module, "SteamAPI_ISteamFriends_GetMediumFriendAvatar"));
    const auto size_fn = reinterpret_cast<bool (*)(void*, int, std::uint32_t*, std::uint32_t*)>(GetProcAddress(module, "SteamAPI_ISteamUtils_GetImageSize"));
    const auto rgba_fn = reinterpret_cast<bool (*)(void*, int, std::uint8_t*, int)>(GetProcAddress(module, "SteamAPI_ISteamUtils_GetImageRGBA"));
    if (!friends_fn || !utils_fn || !avatar_fn || !size_fn || !rgba_fn) return nullptr;
    auto friends = friends_fn();
    auto utils = utils_fn();
    if (!friends || !utils) return nullptr;
    const int image = avatar_fn(friends, steam_id);
    std::uint32_t width{}, height{};
    if (image <= 0 || !size_fn(utils, image, &width, &height) || !width || !height || width > 1024 || height > 1024) return nullptr;
    std::vector<std::uint8_t> rgba(width * height * 4);
    if (!rgba_fn(utils, image, rgba.data(), static_cast<int>(rgba.size()))) return nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = rgba.data();
    data.SysMemPitch = width * 4;
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(device->CreateTexture2D(&desc, &data, texture.GetAddressOf()))) return nullptr;
    if (FAILED(device->CreateShaderResourceView(texture.Get(), nullptr, entry.texture.GetAddressOf()))) return nullptr;
    return entry.texture.Get();
}

//pasted 
static void spectator_list()
{
	if (!config::context.spec_list) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	if (!sdk::local_controller || !interfaces::entity_list) return;

	auto local = sdk::local_controller;
    auto schema_name = [](entity_instance_t* entity) -> const char* {
        void* binding = nullptr;
        utilities::call_virtual<47U, void>(entity, &binding);
        return binding ? *reinterpret_cast<const char**>(reinterpret_cast<std::uint8_t*>(binding) + 8) : nullptr;
    };

    auto observer_services = [&](controller_t* controller) -> player_observer_services_t* {
        const entity_handle_t handles[] = { controller->observer_pawn(), controller->pawn_handle() };
        for (const auto handle : handles) {
            if (!handle || handle == 0xffffffffu) continue;
            auto pawn = reinterpret_cast<base_player_pawn_t*>(interfaces::entity_list->get_entity_from_handle(handle));
            if (!pawn) continue;
            auto identity = pawn->entity_identity();
            const auto type = schema_name(pawn);
            const bool valid_handle = identity && identity->handle == handle;
            const bool valid_type = type && (!std::strcmp(type, "C_CSPlayerPawn") || !std::strcmp(type, "C_CSObserverPawn"));
            if (!valid_handle || !valid_type) continue;
            auto services = pawn->observer_services();
            if (!services) continue;
            if (!interfaces::entity_list->get_entity_from_handle(services->observer_target())) continue;
            return services;
        }
        return nullptr;
    };
    auto view_pawn = sdk::local_player;
    if (!view_pawn || !view_pawn->is_alive()) {
        auto services = observer_services(local);
        view_pawn = services ? reinterpret_cast<player_t*>(interfaces::entity_list->get_entity_from_handle(services->observer_target())) : nullptr;
    }
    const bool all = config::context.spec_all;
    if (!view_pawn && !all) return;
    auto view_controller = view_pawn ? interfaces::entity_list->get_entity_from_handle(view_pawn->controller_handle()) : nullptr;
    struct spectator_entry { std::string name, target; std::uint64_t steam_id{}; };
    spectator_entry entries[64]{};
    int count = 0;
    for (int i = 1; i <= 64 && count < 64; ++i) {
        auto entity = reinterpret_cast<entity_t*>(interfaces::entity_list->get_base_entity(i));
        if (!entity) continue;
        const auto type = schema_name(entity);
        if (!type || std::strcmp(type, "CCSPlayerController")) continue;
        auto controller = reinterpret_cast<controller_t*>(entity);
        if (!controller || (!all && (controller == local || controller == view_controller))) continue;
        if (controller->is_alive()) continue;
        auto services = observer_services(controller);
        if (!services) continue;
        auto target = reinterpret_cast<player_t*>(interfaces::entity_list->get_entity_from_handle(services->observer_target()));
        if (!target || (!all && target != view_pawn)) continue;
        const auto target_type = schema_name(target);
        if (!target_type || (std::strcmp(target_type, "C_CSPlayerPawn") && std::strcmp(target_type, "C_CSObserverPawn"))) continue;
        auto target_controller = reinterpret_cast<controller_t*>(interfaces::entity_list->get_entity_from_handle(target->controller_handle()));
        const auto controller_type = target_controller ? schema_name(target_controller) : nullptr;
        if (!controller_type || std::strcmp(controller_type, "CCSPlayerController")) continue;
        const char* name = controller->name();
        if (!name) continue;
        auto& entry = entries[count++];
        entry.name.assign(name, strnlen_s(name, 127));
        const auto target_name = target_controller->name();
        entry.target.assign(target_name ? target_name : "unknown", target_name ? strnlen_s(target_name, 127) : 7);
        entry.steam_id = controller->steam_id();
    }
    if (!count && config::context.spec_list_hide_empty) return;
    if (menu::font) ImGui::PushFont(menu::font);
    const auto title = ImGui::CalcTextSize("spectators");
    const int visible = (std::max)(count, 1);
    const auto display = ImGui::GetIO().DisplaySize;
    const float width = (std::min)(all ? 360.f : 220.f, (std::max)(100.f, display.x));
    constexpr float header_height = menu::overlay::header_height;
    constexpr float row_height = menu::overlay::row_height + 2.f;
    const float height = (std::min)(header_height + visible * row_height + 8.f, (std::max)(header_height + row_height, display.y - 8.f));
    auto& pos = config::context.speclist_pos;
    if (pos.x < 0.f || pos.y < 0.f) pos = vec2_t((display.x - width) * 0.5f, (display.y - height) * 0.5f);
    menu::overlay::clamp_position(pos, ImVec2(width, height));
    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove;
    if (!menu::open) flags |= ImGuiWindowFlags_NoInputs;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    if (ImGui::Begin("##spectators", nullptr, flags)) {
        menu::overlay::drag_header(pos, ImVec2(width, height), menu::open);
        const auto origin = ImGui::GetWindowPos();
        const float x = std::floor(origin.x);
        float y = std::floor(origin.y);
        auto draw = ImGui::GetWindowDrawList();
        auto packed_color = [](float (&rgba)[4]) {
            const bool legacy = rgba[0] > 1.f || rgba[1] > 1.f || rgba[2] > 1.f || rgba[3] > 1.f;
            if (legacy) for (auto& channel : rgba) channel /= 255.f;
            return ImGui::ColorConvertFloat4ToU32(ImVec4(rgba[0], rgba[1], rgba[2], rgba[3]));
        };
        const ImU32 accent = packed_color(config::context.spec_name_color);
        const ImU32 pl_accent = packed_color(config::context.spec_player_name_color);
        const auto& menu_accent = config::context.menu_accent;
        menu::overlay::frame("spectators", ImVec2(x, y), ImVec2(width, height), accent,
            ImGui::ColorConvertFloat4ToU32(ImVec4(menu_accent[0], menu_accent[1], menu_accent[2], 1.f)));
        char count_text[8]{};
        std::snprintf(count_text, sizeof(count_text), "%d", count);
        const auto count_size = ImGui::CalcTextSize(count_text);
        draw->AddText(ImVec2(x + width - 12.f - count_size.x, y + 8.f), IM_COL32(153, 153, 153, 255), count_text);
        ImGui::SetCursorPos(ImVec2(0.f, header_height + 3.f));
        ImGui::BeginChild("##spectator_rows", ImVec2(width, height - header_height - 7.f), false, ImGuiWindowFlags_NoBackground);
        draw = ImGui::GetWindowDrawList();
        if (!count) {
            const auto row_pos = ImGui::GetCursorScreenPos();
            draw->AddText(ImVec2(row_pos.x + 11.f, row_pos.y + 4.f), menu::overlay::muted, "no spectators");
            ImGui::Dummy(ImVec2(0.f, row_height));
        }
        for (int i = 0; i < count; ++i) {
            const auto row_pos = ImGui::GetCursorScreenPos();
            const float row_width = ImGui::GetContentRegionAvail().x;
            const float row_x = row_pos.x;
            y = row_pos.y;
            auto avatar = spectator_avatar(entries[i].steam_id);
            const float split = row_x + row_width * .55f;
            const float name_width = all ? split - row_x - 44.f : row_width - 47.f;
            const auto name = menu::overlay::fit_text(entries[i].name.c_str(), name_width);
            if (i & 1) draw->AddRectFilled(ImVec2(row_x + 3.f, y), ImVec2(row_x + row_width - 3.f, y + row_height), menu::overlay::section_background);
            draw->AddRectFilled(ImVec2(row_x + 11.f, y + 3.f), ImVec2(row_x + 29.f, y + 21.f), IM_COL32(35, 35, 41, 255), 4.f);
            if (avatar) {
                draw->AddImageRounded(reinterpret_cast<ImTextureID>(avatar), ImVec2(row_x + 11.f, y + 3.f), ImVec2(row_x + 29.f, y + 21.f), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), IM_COL32_WHITE, 4.f);
            } else {
                char initial[2]{ entries[i].name.empty() || static_cast<unsigned char>(entries[i].name[0]) >= 128 ? '?' : entries[i].name[0], 0 };
                const auto letter_size = ImGui::CalcTextSize(initial);
                draw->AddText(ImVec2(row_x + 20.f - letter_size.x * .5f, y + (row_height - letter_size.y) * .5f), menu::overlay::muted, initial);
            }
            draw->AddText(ImVec2(row_x + 36.f, y + (row_height - ImGui::GetFontSize()) * .5f), pl_accent, name.c_str());
            if (all) {
                draw->AddText(ImVec2(split - 4.f, y + (row_height - ImGui::GetFontSize()) * .5f), menu::overlay::muted, ">");
                const auto target = menu::overlay::fit_text(entries[i].target.c_str(), row_width - (split - row_x) - 23.f);
                draw->AddText(ImVec2(split + 10.f, y + (row_height - ImGui::GetFontSize()) * .5f), menu::overlay::text, target.c_str());
            }
            ImGui::Dummy(ImVec2(row_width, row_height));
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s -> %s", entries[i].name.c_str(), entries[i].target.c_str());
        }
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (menu::font) ImGui::PopFont();
}

static void practice_list()
{
	if (!config::context.practice || !config::context.practice_list) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	if (menu::font) ImGui::PushFont(menu::font);
	constexpr ImVec2 size(200.f, menu::overlay::header_height + menu::overlay::row_height * 2.f + 8.f);
	auto& pos = config::context.practice_list_pos;
	menu::overlay::clamp_position(pos, size);
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(size, ImGuiCond_Always);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove;
	if (!menu::open) flags |= ImGuiWindowFlags_NoInputs;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	if (ImGui::Begin("##practice_list", nullptr, flags)) {
		menu::overlay::drag_header(pos, size, menu::open);
		const auto origin = ImGui::GetWindowPos();
		const auto& color = config::context.menu_accent;
		menu::overlay::frame("practice", origin, size, IM_COL32(220, 220, 220, 255),
			ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], 1.f)));
		const bool saved = practice_saved.load(std::memory_order_relaxed);
		auto draw = ImGui::GetWindowDrawList();
		menu::overlay::row(draw, ImVec2(origin.x, origin.y + menu::overlay::header_height + 3.f), size.x,
			"save", menu::key_to_string(config::context.practice_save_key));
		menu::overlay::row(draw, ImVec2(origin.x, origin.y + menu::overlay::header_height + menu::overlay::row_height + 3.f), size.x,
			"load", menu::key_to_string(config::context.practice_load_key), saved ? menu::overlay::text : menu::overlay::muted);
	}
	ImGui::End();
	ImGui::PopStyleVar(2);
	if (menu::font) ImGui::PopFont();
}

void misc::grenade_trail() {
	if (!interfaces::cvar) return;
	if (!interfaces::engine || !interfaces::engine->is_in_game()) return;
	auto cvar = interfaces::cvar->FindVar("sv_grenade_trajectory_prac_trailtime");
	if (!cvar) return;
	const bool want = config::context.grenade_trail;

	if (want && cvar->get_float() == 0.f)
		cvar->set_float(1.f);
	else if (!want && cvar->get_float() != 0.f)
		cvar->set_float(0.f);
}

void misc::grenade_preview()
{
    if (!interfaces::cvar) return;
    auto cvar = interfaces::cvar->FindVar("sv_grenade_trajectory_prac_pipreview");
    if (!cvar) return;
    const bool want = config::context.grenade_preview;
    if (cvar->get_bool() != want) cvar->set_bool(want);
}

void misc::run()
{
	practice_input();
	grenade_trail();
    grenade_preview();
	draw_watermark();
	velocityind();
	keystrokes();
	indicators();
	spectator_list();
	practice_list();
}
