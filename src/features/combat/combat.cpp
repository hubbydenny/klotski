#include "combat.hpp"

#include "../config/config.hpp"
#include "../../source2-sdk/sdk.hpp"
#include "../../utilities/utilities.hpp"
#include "../../source2-sdk/interfaces/i_trace.hpp"
#include "../../source2-sdk/interfaces/visible.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <execution>
#include <vector>
#include <cstring>

static bool correct_spread(vec3_t& angle, controller_t* controller)
{
    const auto failed = [](unsigned reason, const char* message) {
        static unsigned reported = 0;
        if (!(reported & reason)) {
            console::log("[-] nospread: %s\n", message);
            reported |= reason;
        }
        return false;
    };
    using seed_fn = std::uint32_t(__fastcall*)(void*, const vec3_t*, int);
    using spread_fn = void(__fastcall*)(std::int16_t,int,int,std::uint32_t,float,float,float,float*,float*);
    using accuracy_fn = float(__fastcall*)(void*,float*,float*);
    using cone_fn = float(__fastcall*)(void*);
    using update_accuracy_fn = void(__fastcall*)(void*);
    static auto seed = reinterpret_cast<seed_fn>(utilities::pattern_scan(L"client.dll", "48 89 5C 24 08 57 48 81 EC F0 00 00 00 F3 0F 10 0A 48 8D 8C 24 10 01 00 00 41 8B D8 48 8B FA E8 ? ? ? ? F3 0F 10 4F 04"));
    static auto spread = [] {
        auto call = utilities::pattern_scan(L"client.dll", "28 F3 44 0F 11 44 24 20 E8 ? ? ? ? 48 8D 85 B0 00 00 00");
        return call ? reinterpret_cast<spread_fn>(utilities::resolve_rip(call + 8,1,5)) : nullptr;
    }();
    static auto accuracy = reinterpret_cast<accuracy_fn>(utilities::pattern_scan(L"client.dll", "48 89 5C 24 ? 55 56 57 48 81 EC ? ? ? ? 44 0F 29 84 24"));
    static auto cone = reinterpret_cast<cone_fn>(utilities::pattern_scan(L"client.dll", "48 63 91 00 1A 00 00 48 8B 81 88 03 00 00 85 D2 78 ? 48 83 FA 02 73 ? F3 0F 10 84 90 50 07 00 00"));
    static auto update_accuracy = reinterpret_cast<update_accuracy_fn>(utilities::pattern_scan(L"client.dll", "40 57 41 56 48 83 EC 68 48 8B F9 E8 ? ? ? ? 4C 8B F0 48 85 C0"));
    if (!seed) return failed(1, "spread seed signature not found");
    if (!spread) return failed(2, "calculate spread signature not found");
    if (!accuracy) return failed(4, "inaccuracy signature not found");
    if (!cone) return failed(8, "weapon spread signature not found");
    if (!update_accuracy) return failed(256, "update accuracy signature not found");
    if (!interfaces::entity_list) return false;
    auto local = sdk::local_player;
    auto services = local->weapon_services();
    if (!services) return false;
    auto weapon = static_cast<cs_weapon_base_t*>(interfaces::entity_list->get_entity_from_handle(services->active_weapon()));
    if (!weapon || weapon->clip() <= 0 || weapon->next_primary_attack_tick() > static_cast<int>(controller->tick_base())) return false;
    auto data = weapon->get_vdata();
    if (!data) return failed(16, "weapon vdata missing");
    if (!schema_system::has_schema("CCSWeaponBaseVData", "m_nNumBullets"))
        return failed(32, "weapon projectile count schema missing");
    const int num_bullets = data->num_bullets();
    if (num_bullets < 1 || num_bullets > 64)
        return failed(512, "invalid weapon projectile count");
    const auto manager = schema_system::get_schema("C_EconEntity", "m_AttributeManager");
    const auto item = schema_system::get_schema("C_AttributeContainer", "m_Item");
    const auto begin = schema_system::get_schema("C_CSWeaponBase", "m_flTurningInaccuracyDelta");
    const auto end = schema_system::get_schema("C_CSWeaponBase", "m_flRecoilIndex");
    if (!manager || !item || !begin || end < begin || end - begin + sizeof(float) > 256)
        return failed(64, "weapon accuracy schema unavailable or layout mismatch");
    const auto definition = reinterpret_cast<econ_item_view_t*>(reinterpret_cast<std::uintptr_t>(weapon)+manager+item)->item_definition_index();
    std::array<std::uint8_t,256> backup{};
    auto state = reinterpret_cast<std::uint8_t*>(weapon) + begin;
    const auto size = end - begin + sizeof(float);
    std::memcpy(backup.data(),state,size);
    update_accuracy(weapon);
    const float inaccuracy = accuracy(weapon,nullptr,nullptr);
    const float spread_value = cone(weapon);
    const float recoil = weapon->recoil_index();
    std::memcpy(state,backup.data(),size);
    if (!std::isfinite(inaccuracy) || !std::isfinite(spread_value) || !std::isfinite(recoil) || inaccuracy < 0.f || spread_value < 0.f) return false;
    const auto punch = local->get_aim_punch();
    const auto wanted = angle;
    constexpr float deg = 180.f / 3.14159265358979323846f;
    auto seed_of = [&](const vec3_t& candidate) {
        return seed(nullptr,&candidate,static_cast<int>(controller->tick_base()));
    };
    auto spread_of = [&](std::uint32_t value) {
        vec2_t out{};
        spread(static_cast<std::int16_t>(definition),num_bullets,0,value+1,inaccuracy,spread_value,recoil,&out.x,&out.y);
        return out;
    };
    auto correct_for = [&](std::uint32_t value) {
        const auto offset = spread_of(value);
        return vec3_t(wanted.x + std::atan(std::hypot(offset.x,offset.y))*deg,wanted.y,-std::atan2(offset.x,offset.y)*deg);
    };
    auto accept = [&](const vec3_t& candidate) {
        if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) return false;
        angle = vec3_t(candidate.x-punch.x,candidate.y-punch.y,candidate.z);
        return true;
    };
    for (int i=0;i<720;++i) {
        const auto trial = seed_of(vec3_t(i*.5f,wanted.y,0.f));
        const auto candidate = correct_for(trial);
        if (seed_of(candidate) == trial) return accept(candidate);
    }
    static std::array<bool, 128> reported_weapon{};
    if (definition < reported_weapon.size() && !reported_weapon[definition]) {
        console::log("[-] nospread: no exact seed found; shot skipped id=%u pellets=%d inaccuracy=%.6f spread=%.6f recoil=%.3f\n",
            static_cast<unsigned>(definition), num_bullets, inaccuracy, spread_value, recoil);
        reported_weapon[definition] = true;
    }
    return false;
}

static void cancel_spread_shot(user_cmd_t* cmd)
{
    cmd->set_button(in_attack, false);
    for (int i = 0; i < cmd->subtick_count(); ++i) {
        auto step = cmd->get_subtick(i);
        if (step && (step->button & in_attack)) {
            step->set_button(step->button & ~static_cast<std::uint64_t>(in_attack));
            if (!step->button) step->set_pressed(false);
        }
    }
    cmd->cmd.attack1_start_history_index = -1;
}

static float compute_rcs_factor(int rand_min, int rand_max);
struct aim_snapshot_t
{
	player_t* player{};
	std::array<vec3_t, aim_bone_count + 1> positions{};
	std::size_t count{};
	vec3_t best_position{};
	float fov{};
};

static aim_snapshot_t capture_target(player_t* player)
{
	aim_snapshot_t snapshot{};
	snapshot.player = player;
	player->update_bones();

	std::int32_t bone_count = 0;
	bone_data_t* bones = player->get_bone_cache(&bone_count);

	if (bones && bone_count > 0)
	{
		bool selected = false;

		for (std::uint32_t i = 0; i < aim_bone_count; i++)
		{
			if (!config::context.aim_bones[i]) continue;

			selected = true;

			const std::int32_t bone = static_cast<std::int32_t>(aim_bones_table[i].bone);

			if (bone >= bone_count) continue;

			const vec3_t position = bones[bone].position;

			if (position.is_zero()) continue;

			snapshot.positions[snapshot.count++] = position;
		}

		if (!selected)
		{
			const std::int32_t head = static_cast<std::int32_t>(bone_ids::head);

			if (head >= bone_count) return snapshot;

			const vec3_t position = bones[head].position;

			if (!position.is_zero()) snapshot.positions[snapshot.count++] = position;
		}

		return snapshot;
	}

	vec3_t fallback{};
	if (!player->try_abs_origin(fallback)) return snapshot;

	fallback.z += player->view_offset().z;

	snapshot.positions[snapshot.count++] = fallback;
	return snapshot;
}

void combat::run_rcs()
{
	if (!interfaces::csgo_input || !interfaces::engine || !interfaces::engine->is_in_game()) return;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return;

	vec3_t* view_angles = interfaces::csgo_input->get_view_angles(0);

	if (!view_angles) return;

	const vec3_t aim_punch = sdk::local_player->get_aim_punch();

	if (config::context.standalone_rcs)
	{
		combat::update_standalone_rcs(
			*view_angles,
			aim_punch,
			config::context.standalone_rcs_strength,
			config::context.standalone_rcs_min,
			config::context.standalone_rcs_max,
			true
		);
	}
}

bool combat::run_aimbot(controller_t* local_controller, user_cmd_t* cmd) {
	if (!cmd || !local_controller || !interfaces::csgo_input || !interfaces::engine || !interfaces::entity_list) return false;
	if (!interfaces::engine->is_in_game()) return false;
	if (!sdk::local_player || !sdk::local_player->is_alive()) return false;

	vec3_t* view_angles = interfaces::csgo_input->get_view_angles(0);

	if (!view_angles) return false;
	const vec3_t local_eye_position = sdk::local_player->eye_position();

	float best_fov = config::context.aimbot_fov;
	vec3_t best_position = vec3_t();
	player_t* best_target = nullptr;
	thread_local std::vector<aim_snapshot_t> targets;
	targets.clear();
	targets.reserve(64);

	for (std::int32_t i = 1; i <= 64; i++)
	{
		controller_t* controller = interfaces::entity_list->get_controller_by_index(i);

		if (!controller) continue;

		player_t* player = interfaces::entity_list->get_player_from_controller(controller);

		if (!player || player == sdk::local_player) continue;
		if (!player->is_alive() || player->has_gun_immunity()) continue;
		if (player->team() == sdk::local_player->team()) continue;

		game_scene_node_t* scene_node = player->game_scene_node();
		if (!scene_node || scene_node->dormant()) continue;
		if (config::context.aimbot_max_distance > 0.f)
		{
			vec3_t origin{};
			if (!player->try_abs_origin(origin)) continue;
			const float dx = origin.x - local_eye_position.x;
			const float dy = origin.y - local_eye_position.y;
			const float dz = origin.z - local_eye_position.z;
			const float distance_sq = dx * dx + dy * dy + dz * dz;
			const float max_distance = config::context.aimbot_max_distance;

			if (distance_sq > max_distance * max_distance) continue;
		}

		targets.push_back(capture_target(player));
	}

	const vec3_t captured_angles = *view_angles;
	const auto rank = [local_eye_position, captured_angles, best_fov](aim_snapshot_t& target) {
		target.fov = best_fov;
		for (std::size_t i = 0; i < target.count; ++i) {
			const float fov = math::angle_distance(local_eye_position, target.positions[i], captured_angles);
			if (std::isfinite(fov) && fov < target.fov) {
				target.fov = fov;
				target.best_position = target.positions[i];
			}
		}
	};
	// Workers only process owned coordinates; engine calls stay on the caller.
	if (targets.size() >= 16) std::for_each(std::execution::par, targets.begin(), targets.end(), rank);
	else std::for_each(targets.begin(), targets.end(), rank);
	std::stable_sort(targets.begin(), targets.end(), [](const auto& a, const auto& b) { return a.fov < b.fov; });
	for (const auto& target : targets) {
		if (target.best_position.is_zero()) continue;
		if (config::context.onlyvisible && !is_visible(sdk::local_player, target.player, target.best_position)) continue;
		best_position = target.best_position;
		best_target = target.player;
		break;
	}

	if (!best_target || best_position.is_zero()) return false;
	vec3_t aim_delta = math::calculate_angle(local_eye_position, best_position, *view_angles);
	if (config::context.standalone_rcs && sdk::local_player->shots_fired() > 1)
	{
		const float scale = static_cast<float>(config::context.standalone_rcs_strength) / 100.f;
		const float factor = compute_rcs_factor(config::context.standalone_rcs_min, config::context.standalone_rcs_max);
		const vec3_t aim_punch = sdk::local_player->get_aim_punch();
		aim_delta.x -= aim_punch.x * scale * factor;
		aim_delta.y = normalize_yaw(aim_delta.y - aim_punch.y * scale * factor);
	}

	if (config::context.aimbot_smooth && !config::context.silent_aim)
	{
		const float speed = config::context.aimbot_speed / 100.0f;

		aim_delta.x *= speed;
		aim_delta.y *= speed;
		aim_delta.z = 0.0f;
	}

	vec3_t new_angles = *view_angles + aim_delta;
	new_angles.clamp();

	if (config::context.no_spread && (cmd->buttons & in_attack)) {
        auto corrected = math::calculate_angle(local_eye_position, best_position, vec3_t());
        if (correct_spread(corrected, local_controller)) {
            const bool applied = cmd->set_input_history(corrected, local_controller, best_target);
            static bool reported_history = false;
            if (!applied && !reported_history) {
                console::log("[-] nospread: command has no writable input history\n");
                reported_history = true;
            }
            if (!applied) cancel_spread_shot(cmd);
            return applied;
        }
        cancel_spread_shot(cmd);
        return false;
    }
    const bool history_applied = cmd->set_input_history(new_angles, local_controller, best_target, false, true);
    if (config::context.silent_aim) return history_applied;
    auto base = cmd->get_base();
    if (!base || !base->view_angles) return false;
    base->view_angles->angles = new_angles;
    base->has_bits |= cbase_user_cmd_pb_t::has_viewangles;
    interfaces::csgo_input->set_view_angles(new_angles);
    return true;
}
static float compute_rcs_factor(int rand_min, int rand_max)
{
	const float current_time = interfaces::globals ? interfaces::globals->curtime : 0.0f;

	const auto seed = static_cast<std::uint32_t>(current_time * 1000.0f);
	const auto t = static_cast<float>(seed % 1000) / 1000.0f;

	const auto min_scale = static_cast<float>(rand_min) / 100.0f;
	const auto max_scale = static_cast<float>(rand_max) / 100.0f;

	return min_scale + (max_scale - min_scale) * t;
}

static vec3_t g_old_punch{};

void combat::update_standalone_rcs(const vec3_t& view_angles, const vec3_t& aim_punch, int amount, int rand_min, int rand_max, bool apply)
{
	if (!sdk::local_player || !interfaces::csgo_input) return;

	const std::int32_t shots_fired = sdk::local_player->shots_fired();

	if (shots_fired > 1)
	{
		const auto factor = compute_rcs_factor(rand_min, rand_max);
		const auto scale = static_cast<float>(amount) / 100.0f;

		vec3_t punch_scaled
		{
			aim_punch.x * scale * factor,
			aim_punch.y * scale * factor,
			0.0f
		};

		if (apply)
		{
			vec3_t new_angles = view_angles;
			new_angles.x += g_old_punch.x - punch_scaled.x;
			new_angles.y += g_old_punch.y - punch_scaled.y;
			new_angles.clamp();
			interfaces::csgo_input->set_view_angles(new_angles);
		}

		g_old_punch = punch_scaled;
	}
	else
	{
		g_old_punch = vec3_t();
	}
}



