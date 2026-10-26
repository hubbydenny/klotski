module;

#include <algorithm>
#include <cmath>
#include <limits>
#include "../../utilities/utilities.hpp"

module class_user_cmd;
import players_class;
import math;

bool user_cmd_t::set_input_history(const vec3_t& angles, controller_t* controller, player_t* player, bool auto_shoot, bool preserve_timing) {
	if (!controller || !player) return false;
	if (!std::isfinite(angles.x) || !std::isfinite(angles.y) || !std::isfinite(angles.z)) return false;

	constexpr float interval = 1.f / 64.f;
	const float simulation_time = preserve_timing ? 0.f : player->simulation_time();
	if (!std::isfinite(simulation_time) || simulation_time < 0.f) return false;
	const double render_tick = std::floor(0.5 + static_cast<double>(simulation_time) / interval) + 1.0;
	const auto tick_base = controller->tick_base();
	if (render_tick > (std::numeric_limits<std::int32_t>::max)() || tick_base > static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)())) return false;

	auto& history = cmd.input_history;
	auto rep = history.rep;
	if (!rep || !utilities::is_readable(rep, sizeof(protobuf_repeated_ptr_field_t::rep_t))) return false;
	const int count = (std::min)({ history.current_size, history.capacity, rep->allocated_size });
	if (count <= 0 || !utilities::is_readable(rep->elements, sizeof(void*) * static_cast<std::size_t>(count))) return false;

	const float fraction = auto_shoot ? std::nextafterf(1.f, 0.f) : 0.f;
	bool changed = false;
	for (int i = 0; i < count; i++)
	{
		auto entry = static_cast<csgo_input_history_entry_pb_t*>(rep->elements[i]);
		if (!entry || !utilities::is_readable(entry, sizeof(csgo_input_history_entry_pb_t))) continue;
		if (!entry->view_angles || !utilities::is_readable(entry->view_angles, sizeof(cmd_qangle_t))) continue;

		entry->view_angles->angles = angles;
        entry->has_bits |= csgo_input_history_entry_pb_t::has_viewangles;
        changed = true;
        if (preserve_timing) continue;
		entry->player_tick_count = static_cast<std::int32_t>(tick_base);
		entry->player_tick_fraction = fraction;
		entry->render_tick_count = static_cast<std::int32_t>(render_tick);
		entry->render_tick_fraction = 0.f;
		entry->has_bits |= csgo_input_history_entry_pb_t::has_viewangles
			| csgo_input_history_entry_pb_t::has_player_tick_count
			| csgo_input_history_entry_pb_t::has_player_tick_fraction
			| csgo_input_history_entry_pb_t::has_render_tick_count
			| csgo_input_history_entry_pb_t::has_render_tick_fraction;
		changed = true;
	}
	return changed;
}
