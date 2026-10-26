#include "prediction.hpp"
#include "../../../includes.hpp"

#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

// Should be fully recoded
namespace {
	using prediction_context = ::ctx::prediction_context;
	using interface_callback_fn = void*(__cdecl*)();

	class state_guard {
	public:
		~state_guard()
		{
			restore();
		}

		template <typename T>
		void save(T& value)
		{
			save_raw(&value, sizeof(T));
		}

		void save_raw(void* address, std::size_t size)
		{
			if (!address || size == 0) return;

			const auto offset = data.size();
			data.resize(offset + size);
			entries.push_back({ reinterpret_cast<std::uintptr_t>(address), offset, size });
			std::memcpy(data.data() + offset, address, size);
		}

		void save_raw(std::uintptr_t address, std::size_t size)
		{
			save_raw(reinterpret_cast<void*>(address), size);
		}

		void restore()
		{
			for (auto it = entries.rbegin(); it != entries.rend(); ++it) std::memcpy(reinterpret_cast<void*>(it->address), data.data() + it->offset, it->size);
			entries.clear();
			data.clear();
		}

	private:
		struct entry
		{
			std::uintptr_t address{};
			std::size_t offset{};
			std::size_t size{};
		};

		std::vector<entry> entries{};
		std::vector<std::uint8_t> data{};
	};


	bool save_schema(state_guard& guard, void* base, const char* class_name, const char* property_name, std::size_t size)
	{
		if (!base) return false;

		const std::uint32_t offset = schema_system::get_schema(class_name, property_name);
		if (offset == 0) return false;

		guard.save_raw(reinterpret_cast<std::uint8_t*>(base) + offset, size);
		return true;
	}

	void save_globals(state_guard& guard, global_vars_t* globals)
	{
		if (!globals) return;

		std::uint8_t* base = reinterpret_cast<std::uint8_t*>(globals);
		guard.save_raw(base + 0x30, sizeof(float));
		guard.save_raw(base + 0x34, sizeof(float));
		guard.save_raw(base + 0x38, sizeof(float));
		guard.save_raw(base + 0x3C, sizeof(float));
		guard.save_raw(base + 0x40, sizeof(float));
		guard.save_raw(base + 0x44, sizeof(std::int32_t));
		guard.save_raw(base + 0x48, sizeof(std::int32_t));
		guard.save_raw(base + 0x50, sizeof(float));
		guard.save_raw(base + 0x58, sizeof(std::uint32_t));
	}

	void save_command(state_guard& guard, user_cmd_t* cmd)
	{
		if (!cmd) return;

		guard.save_raw(cmd, sizeof(user_cmd_t));

		cbase_user_cmd_pb_t* base = cmd->get_base();
		if (!base) return;

		guard.save_raw(base, sizeof(cbase_user_cmd_pb_t));

		if (base->buttons) guard.save_raw(base->buttons, sizeof(cin_button_state_pb_t));
		if (base->view_angles) guard.save_raw(base->view_angles, sizeof(cmd_qangle_t));

		protobuf_repeated_ptr_field_t& field = base->subtick_moves;
		protobuf_repeated_ptr_field_t::rep_t* rep = field.rep;
		if (!rep) return;

		const std::int32_t count = (std::min)(field.current_size, (std::min)(field.capacity, rep->allocated_size));
		for (std::int32_t i = 0; i < count; i++)
		{
			subtick_move_step_t* step = cmd->get_subtick(i);
			if (step) guard.save_raw(step, sizeof(subtick_move_step_t));
		}
	}

	void save_state(state_guard& guard, const prediction_context& p_ctx, bool command)
	{
		save_globals(guard, p_ctx.globals);

		if (p_ctx.prediction)
		{
			guard.save(p_ctx.prediction->in_prediction);
			guard.save(p_ctx.prediction->first_prediction);
		}

		if (p_ctx.network) guard.save(p_ctx.network->m_bShouldPredict);
		if (p_ctx.cmd)
		{
			guard.save_raw(reinterpret_cast<std::uint8_t*>(p_ctx.cmd) + 0x88, sizeof(bool));
			guard.save(p_ctx.cmd->subtick_state);
		}

		if (p_ctx.controller) guard.save(p_ctx.controller->tick_base());
		if (p_ctx.player)
		{
			guard.save(p_ctx.player->flags());
			save_schema(guard, p_ctx.player, "C_BaseEntity", "m_MoveType", sizeof(std::uint8_t));
			save_schema(guard, p_ctx.player, "C_BaseEntity", "m_nActualMoveType", sizeof(std::uint8_t));
			guard.save(p_ctx.player->velocity());
			guard.save(p_ctx.player->abs_velocity());
			guard.save(p_ctx.player->base_velocity());
			guard.save(p_ctx.player->ground_entity());
			guard.save(p_ctx.player->simulation_time());
			guard.save(p_ctx.player->view_offset());
			guard.save(p_ctx.player->old_origin());
			guard.save(p_ctx.player->eye_angles());
			save_schema(guard, p_ctx.player, "C_BasePlayerPawn", "m_vecLastCameraSetupLocalOrigin", sizeof(vec3_t));
			save_schema(guard, p_ctx.player, "C_BasePlayerPawn", "m_flLastCameraSetupTime", sizeof(float));
			save_schema(guard, p_ctx.player, "C_BasePlayerPawn", "m_vecPredictionError", sizeof(vec3_t));
			save_schema(guard, p_ctx.player, "C_BasePlayerPawn", "m_flPredictionErrorTime", sizeof(float));

			const auto collision_offset = schema_system::get_schema("C_BaseModelEntity", "m_Collision");
			if (collision_offset)
			{
				auto collision = reinterpret_cast<std::uint8_t*>(p_ctx.player) + collision_offset;
				save_schema(guard, collision, "CCollisionProperty", "m_vecMins", sizeof(vec3_t));
				save_schema(guard, collision, "CCollisionProperty", "m_vecMaxs", sizeof(vec3_t));
			}

			const auto camera_offset = schema_system::get_schema("C_BasePlayerPawn", "m_pCameraServices");
			if (camera_offset)
			{
				auto camera = *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(p_ctx.player) + camera_offset);
				save_schema(guard, camera, "CPlayer_CameraServices", "m_flOldPlayerZ", sizeof(float));
				save_schema(guard, camera, "CPlayer_CameraServices", "m_flOldPlayerViewOffsetZ", sizeof(float));
			}
		}

		if (p_ctx.scene_node)
		{
			guard.save(p_ctx.scene_node->abs_origin());
			const auto origin_offset = schema_system::get_schema("CGameSceneNode", "m_vecOrigin");
			const auto rotation_offset = schema_system::get_schema("CGameSceneNode", "m_angRotation");
			if (origin_offset && rotation_offset > origin_offset && rotation_offset - origin_offset <= 0x80)
				guard.save_raw(reinterpret_cast<std::uint8_t*>(p_ctx.scene_node) + origin_offset, rotation_offset - origin_offset);
			save_schema(guard, p_ctx.scene_node, "CGameSceneNode", "m_vecWrappedLocalOrigin", sizeof(vec3_t));
		}
		if (p_ctx.movement) {
			guard.save(p_ctx.movement->button_down_mask_prev());
			guard.save(p_ctx.movement->queued_button_down_mask());
			guard.save(p_ctx.movement->queued_button_change_mask());
			guard.save(p_ctx.movement->toggle_button_down_mask());
			guard.save(p_ctx.movement->buttondoublepressed());
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices", "m_nLastCommandNumberProcessed", sizeof(std::uint32_t));
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices", "m_flCmdForwardMove", sizeof(float));
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices", "m_flCmdLeftMove", sizeof(float));
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices", "m_flCmdUpMove", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flStamina", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flAccumulatedJumpError", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bDucked", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bDucking", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bDesiresDuck", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bDuckOverride", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_duckUntilOnGround", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flDuckAmount", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flDuckSpeed", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flDuckRootOffset", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flDuckViewOffset", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flLastDuckTime", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flBombPlantViewOffset", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bUsingGroundTopologyOffset", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flUsingGroundTopologyOffsetTransitionSmoothing", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flHeightAtJumpStart", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_flMaxJumpHeightThisJump", sizeof(float));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_bSpeedCropped", sizeof(bool));
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_vecLastPositionAtFullCrouchSpeed", sizeof(float) * 2);
			save_schema(guard, p_ctx.movement, "CCSPlayer_MovementServices", "m_ModernJump", 56);
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices_Humanoid", "m_flSurfaceFriction", sizeof(float));
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices_Humanoid", "m_flFallVelocity", sizeof(float));
			save_schema(guard, p_ctx.movement, "CPlayer_MovementServices_Humanoid", "m_groundNormal", sizeof(vec3_t));
			guard.save_raw(reinterpret_cast<std::uint8_t*>(p_ctx.movement) + 0x3F8, sizeof(std::uint32_t));
			guard.save_raw(reinterpret_cast<std::uint8_t*>(p_ctx.movement) + 0xD48, sizeof(std::uint64_t));
		}

		if (command) save_command(guard, p_ctx.cmd);
	}

	bool build_context(user_cmd_t* cmd, prediction_context& out)
	{
		if (!cmd) return false;

		out.cmd = cmd;
		out.base = cmd->get_base();
		out.player = sdk::local_player;
		out.controller = sdk::local_controller;
		out.prediction = interfaces::i_pred;
		out.network = reinterpret_cast<INetworkClient*>(interfaces::netclient);
		out.globals = interfaces::globals;

		if (!out.player || !out.controller || !out.player->is_alive()) return false;
		if (!out.prediction || !out.network || !out.globals) return false;

		out.movement = out.player->movement();
		out.scene_node = out.player->game_scene_node();

		return out.movement && out.scene_node;
	}

	void write_prediction_globals(const prediction_context& ctx, std::int32_t tick)
	{
		const float interval = ctx.globals->intervalpertick > 0.f ? ctx.globals->intervalpertick : (1.f / 64.f);
		const float time = static_cast<float>(tick) * interval;
		std::uint8_t* globals = reinterpret_cast<std::uint8_t*>(ctx.globals);

		*reinterpret_cast<float*>(globals + 0x30) = time;
		*reinterpret_cast<float*>(globals + 0x34) = interval;
		*reinterpret_cast<float*>(globals + 0x3C) = interval;
		*reinterpret_cast<float*>(globals + 0x40) = interval;
		*reinterpret_cast<std::int32_t*>(globals + 0x44) = tick;
		*reinterpret_cast<std::int32_t*>(globals + 0x48) = tick;
		*reinterpret_cast<float*>(globals + 0x50) = 0.f;
		*reinterpret_cast<std::uint32_t*>(globals + 0x58) = GetCurrentThreadId();
	}

	bool run_command(const prediction_context& ctx)
	{
		if (ctx.cmd->subtick_state == 3) ctx.cmd->subtick_state = 0;

		ctx.cmd->set_has_been_predicted(false);
		ctx.network->m_bShouldPredict = true;
		ctx.prediction->first_prediction = false;
		ctx.prediction->in_prediction = true;

		ctx.movement->set_prediction_command(ctx.cmd);
		ctx.movement->run_command(ctx.cmd);
		ctx.movement->reset_prediction_command();

		return true;
	}

	state_guard live_guard;
	std::mutex simulation_mutex;
	std::int32_t last_sequence_processed = 0;
	bool live_started = false;
	prediction::prestate_t prestate{};
}

void prediction::capture_prestate()
{
	prestate = {};
	auto local = sdk::local_player;
	if (!local || !local->is_alive() || !local->try_abs_origin(prestate.networked_origin)) return;
	prestate.networked_velocity = local->velocity();
	prestate.flags = local->flags();
	prestate.valid = true;
}

const prediction::prestate_t& prediction::pre()
{
	return prestate;
}

void prediction::start(user_cmd_t* cmd)
{
	if (!cmd) return;
	if (cmd->command_number() == last_sequence_processed) return;

	prediction_context ctx{};
	if (!build_context(cmd, ctx)) return;

	live_guard.restore();
	live_started = false;

	save_state(live_guard, ctx, false);

	if (!run_command(ctx))
	{
		live_guard.restore();
		return;
	}

	live_started = true;
}

void prediction::end(user_cmd_t* cmd)
{
	if (!cmd) return;
	if (!live_started) return;

	last_sequence_processed = cmd->command_number();
	live_guard.restore();
	live_started = false;
}

struct prediction::simulation_t::impl_t
{
	explicit impl_t(user_cmd_t* command, std::size_t count)
		: lock(simulation_mutex), cmd(command), ticks(count)
	{
		if (!cmd || ticks == 0 || ticks > max_simulation_ticks) return;
		if (!build_context(cmd, ctx) || !ctx.base) return;

		save_state(guard, ctx, true);
		guard.save(ctx.movement->step_sound_time());

		if (interfaces::cvar)
		{
			auto landing_sound = interfaces::cvar->FindVar("sv_min_jump_landing_sound");
			auto footsteps = interfaces::cvar->FindVar("sv_footsteps");
			if (landing_sound)
			{
				guard.save(landing_sound->value);
				landing_sound->set_float(1.0e9f);
			}
			if (footsteps)
			{
				guard.save(footsteps->value);
				footsteps->set_int(0);
			}
		}

		initial_command = cmd->command_number_value;
		initial_legacy = ctx.base->legacy_command_number;
		initial_client_tick = ctx.base->client_tick;
		initial_tick_base = ctx.controller->tick_base();
		forward_move = ctx.base->forward_move;
		side_move = ctx.base->side_move;
		up_move = ctx.base->up_move;
		ready = true;
	}

	bool prepare()
	{
		if (!ready || prepared || current_tick >= ticks) return false;

		cmd->command_number_value = initial_command + static_cast<std::int64_t>(current_tick);
		ctx.base->legacy_command_number = initial_legacy + static_cast<std::int32_t>(current_tick);
		ctx.base->client_tick = initial_client_tick + static_cast<std::int32_t>(current_tick);
		ctx.base->forward_move = forward_move;
		ctx.base->side_move = side_move;
		ctx.base->up_move = up_move;

		const std::int32_t predicted_tick = static_cast<std::int32_t>(initial_tick_base + current_tick + 1);
		ctx.controller->tick_base() = predicted_tick;
		write_prediction_globals(ctx, predicted_tick);
		ctx.movement->step_sound_time() = 0.f;
		prepared = true;
		return true;
	}

	bool run()
	{
		if (!prepared) return false;
		prepared = false;
		if (!run_command(ctx))
		{
			ready = false;
			return false;
		}
		current_tick++;
		return true;
	}

	std::unique_lock<std::mutex> lock;
	state_guard guard;
	prediction_context ctx{};
	user_cmd_t* cmd{};
	std::size_t ticks{};
	std::size_t current_tick{};
	std::int64_t initial_command{};
	std::int32_t initial_legacy{};
	std::int32_t initial_client_tick{};
	std::uint32_t initial_tick_base{};
	float forward_move{};
	float side_move{};
	float up_move{};
	bool ready{};
	bool prepared{};
};

prediction::simulation_t::simulation_t(user_cmd_t* cmd, std::size_t ticks)
	: impl(std::make_unique<impl_t>(cmd, ticks))
{
}

prediction::simulation_t::~simulation_t() = default;

bool prediction::simulation_t::valid() const
{
	return impl && impl->ready;
}

bool prediction::simulation_t::prepare_tick()
{
	return impl && impl->prepare();
}

bool prediction::simulation_t::run_tick()
{
	return impl && impl->run();
}

std::size_t prediction::simulation_t::tick() const
{
	return impl ? impl->current_tick : 0;
}
