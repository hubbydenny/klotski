#pragma once

#include "../schema_system/schema_system.hpp"
#include "../../utilities/utilities.hpp"
import trace_class;
import math;
#include "../../utilities/signatures.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"

enum BuiltInInteractionLayer_t
{
	LAYER_INDEX_CONTENTS_SOLID = 0,
	LAYER_INDEX_CONTENTS_HITBOX,
	LAYER_INDEX_CONTENTS_TRIGGER,
	LAYER_INDEX_CONTENTS_SKY,
	LAYER_INDEX_FIRST_USER,
	LAYER_INDEX_NOT_FOUND = -1,
	LAYER_INDEX_MAX_ALLOWED = 64,
};

enum StandardInteractionLayers_t
{
	LAYER_INDEX_CONTENTS_PLAYER_CLIP = LAYER_INDEX_FIRST_USER,
	LAYER_INDEX_CONTENTS_NPC_CLIP,
	LAYER_INDEX_CONTENTS_BLOCK_LOS,
	LAYER_INDEX_CONTENTS_BLOCK_LIGHT,
	LAYER_INDEX_CONTENTS_LADDER,
	LAYER_INDEX_CONTENTS_PICKUP,
	LAYER_INDEX_CONTENTS_BLOCK_SOUND,
	LAYER_INDEX_CONTENTS_NODRAW,
	LAYER_INDEX_CONTENTS_WINDOW,
	LAYER_INDEX_CONTENTS_PASS_BULLETS,
	LAYER_INDEX_CONTENTS_WORLD_GEOMETRY,
	LAYER_INDEX_CONTENTS_WATER,
	LAYER_INDEX_CONTENTS_SLIME,
	LAYER_INDEX_CONTENTS_TOUCH_ALL,
	LAYER_INDEX_CONTENTS_PLAYER,
	LAYER_INDEX_CONTENTS_NPC,
	LAYER_INDEX_CONTENTS_DEBRIS,
	LAYER_INDEX_CONTENTS_PHYSICS_PROP,
	LAYER_INDEX_CONTENTS_NAV_IGNORE,
	LAYER_INDEX_CONTENTS_NAV_LOCAL_IGNORE,
	LAYER_INDEX_CONTENTS_POST_PROCESSING_VOLUME,
	LAYER_INDEX_CONTENTS_UNUSED_LAYER3,
	LAYER_INDEX_CONTENTS_CARRIED_OBJECT,
	LAYER_INDEX_CONTENTS_PUSHAWAY,
	LAYER_INDEX_CONTENTS_SERVER_ENTITY_ON_CLIENT,
	LAYER_INDEX_CONTENTS_CARRIED_WEAPON,
	LAYER_INDEX_CONTENTS_STATIC_LEVEL,
	LAYER_INDEX_FIRST_MOD_SPECIFIC,
};

enum ModSpecificInteractionLayers_t
{
	LAYER_INDEX_CONTENTS_CSGO_TEAM1 = LAYER_INDEX_FIRST_MOD_SPECIFIC,
	LAYER_INDEX_CONTENTS_CSGO_TEAM2,
	LAYER_INDEX_CONTENTS_CSGO_GRENADE_CLIP,
	LAYER_INDEX_CONTENTS_CSGO_DRONE_CLIP,
	LAYER_INDEX_CONTENTS_CSGO_MOVEABLE,
	LAYER_INDEX_CONTENTS_CSGO_OPAQUE,
	LAYER_INDEX_CONTENTS_CSGO_MONSTER,
	LAYER_INDEX_CONTENTS_CSGO_UNUSED_LAYER,
	LAYER_INDEX_CONTENTS_CSGO_THROWN_GRENADE,
};

enum BuiltInCollisionGroup_t
{
	COLLISION_GROUP_ALWAYS = 0,
	COLLISION_GROUP_NONPHYSICAL,
	COLLISION_GROUP_TRIGGER,
	COLLISION_GROUP_CONDITIONALLY_SOLID,
	COLLISION_GROUP_FIRST_USER,
	COLLISION_GROUPS_MAX_ALLOWED = 64,
};

enum StandardCollisionGroups_t
{
	COLLISION_GROUP_DEFAULT = COLLISION_GROUP_FIRST_USER,
	COLLISION_GROUP_DEBRIS,
	COLLISION_GROUP_INTERACTIVE_DEBRIS,
	COLLISION_GROUP_INTERACTIVE,
	COLLISION_GROUP_PLAYER,
	COLLISION_GROUP_BREAKABLE_GLASS,
	COLLISION_GROUP_VEHICLE,
	COLLISION_GROUP_PLAYER_MOVEMENT,
	COLLISION_GROUP_NPC,
	COLLISION_GROUP_IN_VEHICLE,
	COLLISION_GROUP_WEAPON,
	COLLISION_GROUP_VEHICLE_CLIP,
	COLLISION_GROUP_PROJECTILE,
	COLLISION_GROUP_DOOR_BLOCKER,
	COLLISION_GROUP_PASSABLE_DOOR,
	COLLISION_GROUP_DISSOLVING,
	COLLISION_GROUP_PUSHAWAY,
	COLLISION_GROUP_NPC_ACTOR,
	COLLISION_GROUP_NPC_SCRIPTED,
	COLLISION_GROUP_PZ_CLIP,
	COLLISION_GROUP_PROPS,
	LAST_SHARED_COLLISION_GROUP,
};

#define	CONTENTS_EMPTY						0ull

#define CONTENTS_SOLID						( 1ull << LAYER_INDEX_CONTENTS_SOLID )
#define CONTENTS_HITBOX						( 1ull << LAYER_INDEX_CONTENTS_HITBOX )
#define CONTENTS_TRIGGER					( 1ull << LAYER_INDEX_CONTENTS_TRIGGER )
#define CONTENTS_SKY						( 1ull << LAYER_INDEX_CONTENTS_SKY )

#define CONTENTS_PLAYER_CLIP				( 1ull << LAYER_INDEX_CONTENTS_PLAYER_CLIP )
#define CONTENTS_NPC_CLIP					( 1ull << LAYER_INDEX_CONTENTS_NPC_CLIP )
#define CONTENTS_BLOCK_LOS					( 1ull << LAYER_INDEX_CONTENTS_BLOCK_LOS )
#define CONTENTS_BLOCK_LIGHT				( 1ull << LAYER_INDEX_CONTENTS_BLOCK_LIGHT )
#define CONTENTS_LADDER						( 1ull << LAYER_INDEX_CONTENTS_LADDER )
#define CONTENTS_PICKUP						( 1ull << LAYER_INDEX_CONTENTS_PICKUP )
#define CONTENTS_BLOCK_SOUND				( 1ull << LAYER_INDEX_CONTENTS_BLOCK_SOUND )
#define CONTENTS_NODRAW						( 1ull << LAYER_INDEX_CONTENTS_NODRAW )
#define CONTENTS_WINDOW						( 1ull << LAYER_INDEX_CONTENTS_WINDOW )
#define CONTENTS_PASS_BULLETS				( 1ull << LAYER_INDEX_CONTENTS_PASS_BULLETS )
#define CONTENTS_WORLD_GEOMETRY				( 1ull << LAYER_INDEX_CONTENTS_WORLD_GEOMETRY )
#define CONTENTS_WATER						( 1ull << LAYER_INDEX_CONTENTS_WATER )
#define CONTENTS_SLIME						( 1ull << LAYER_INDEX_CONTENTS_SLIME )
#define CONTENTS_TOUCH_ALL					( 1ull << LAYER_INDEX_CONTENTS_TOUCH_ALL )
#define CONTENTS_PLAYER						( 1ull << LAYER_INDEX_CONTENTS_PLAYER )
#define CONTENTS_NPC						( 1ull << LAYER_INDEX_CONTENTS_NPC )
#define CONTENTS_DEBRIS						( 1ull << LAYER_INDEX_CONTENTS_DEBRIS )
#define CONTENTS_PHYSICS_PROP				( 1ull << LAYER_INDEX_CONTENTS_PHYSICS_PROP )
#define CONTENTS_NAV_IGNORE					( 1ull << LAYER_INDEX_CONTENTS_NAV_IGNORE )
#define CONTENTS_NAV_LOCAL_IGNORE			( 1ull << LAYER_INDEX_CONTENTS_NAV_LOCAL_IGNORE )
#define CONTENTS_POST_PROCESSING_VOLUME		( 1ull << LAYER_INDEX_CONTENTS_POST_PROCESSING_VOLUME )
#define CONTENTS_UNUSED_LAYER3				( 1ull << LAYER_INDEX_CONTENTS_UNUSED_LAYER3 )
#define CONTENTS_CARRIED_OBJECT				( 1ull << LAYER_INDEX_CONTENTS_CARRIED_OBJECT )
#define CONTENTS_PUSHAWAY					( 1ull << LAYER_INDEX_CONTENTS_PUSHAWAY )
#define CONTENTS_SERVER_ENTITY_ON_CLIENT	( 1ull << LAYER_INDEX_CONTENTS_SERVER_ENTITY_ON_CLIENT )
#define CONTENTS_CARRIED_WEAPON				( 1ull << LAYER_INDEX_CONTENTS_CARRIED_WEAPON )
#define CONTENTS_STATIC_LEVEL				( 1ull << LAYER_INDEX_CONTENTS_STATIC_LEVEL )

#define CONTENTS_CSGO_TEAM1					( 1ull << LAYER_INDEX_CONTENTS_CSGO_TEAM1 )
#define CONTENTS_CSGO_TEAM2					( 1ull << LAYER_INDEX_CONTENTS_CSGO_TEAM2 )
#define CONTENTS_CSGO_GRENADE_CLIP			( 1ull << LAYER_INDEX_CONTENTS_CSGO_GRENADE_CLIP )
#define CONTENTS_CSGO_DRONE_CLIP			( 1ull << LAYER_INDEX_CONTENTS_CSGO_DRONE_CLIP )
#define CONTENTS_CSGO_MOVEABLE				( 1ull << LAYER_INDEX_CONTENTS_CSGO_MOVEABLE )
#define CONTENTS_CSGO_OPAQUE				( 1ull << LAYER_INDEX_CONTENTS_CSGO_OPAQUE )
#define CONTENTS_CSGO_MONSTER				( 1ull << LAYER_INDEX_CONTENTS_CSGO_MONSTER )
#define CONTENTS_CSGO_UNUSED_LAYER			( 1ull << LAYER_INDEX_CONTENTS_CSGO_UNUSED_LAYER )
#define CONTENTS_CSGO_THROWN_GRENADE		( 1ull << LAYER_INDEX_CONTENTS_CSGO_THROWN_GRENADE )

#define	MASK_ALL					(~0ull)
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYER_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
#define	MASK_NPCSOLID				(CONTENTS_SOLID|CONTENTS_NPC_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_PASS_BULLETS)
#define	MASK_NPCFLUID				(CONTENTS_SOLID|CONTENTS_NPC_CLIP|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC)
#define	MASK_WATER					(CONTENTS_WATER|CONTENTS_SLIME)
#define	MASK_SHOT					(CONTENTS_SOLID|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX)
#define MASK_SHOT_BRUSHONLY			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_DEBRIS)
#define MASK_SHOT_HULL				(CONTENTS_SOLID|CONTENTS_PLAYER|CONTENTS_NPC|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_PASS_BULLETS)
#define MASK_SHOT_PORTAL			(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER|CONTENTS_NPC)
#define MASK_SOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PASS_BULLETS)
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_PLAYER_CLIP|CONTENTS_PASS_BULLETS)
#define MASK_NPCSOLID_BRUSHONLY		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_NPC_CLIP|CONTENTS_PASS_BULLETS)
#define MASK_CLIP_TO_PLAYER (CONTENTS_PLAYER | CONTENTS_HITBOX )
#define MASK_PLAYER_VISIBLE (CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_PLAYER | CONTENTS_NPC | CONTENTS_DEBRIS | CONTENTS_HITBOX | CONTENTS_BLOCK_LOS)
// pasted


constexpr std::uint8_t TRACE_LAYER_DEFAULT = 4;
constexpr std::uint8_t TRACE_FILTER_TYPE = 7;

class i_trace
{
public:
	void clip_trace_to_players(vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace, float min, float max, float length)
	{
		using function_t = void(__fastcall*)(vec3_t&, vec3_t&, trace_filter_t*, game_trace_t*, float, float, float);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", CLIP_TRACE_TO_PLAYERS));
		if (!fn) return;
		fn(start, end, filter, trace, min, max, length);
	}

	bool trace_shape(ray_t* ray, vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace) { return trace_shape_world(this, ray, start, end, filter, trace); }
	static bool trace_shape_world(i_trace* world, ray_t* ray, vec3_t& start, vec3_t& end, trace_filter_t* filter, game_trace_t* trace)
	{
		using function_t = bool(__fastcall*)(i_trace*, ray_t*, vec3_t&, vec3_t&, trace_filter_t*, game_trace_t*);
		function_t fn = reinterpret_cast<function_t>(get_trace_shape());

		if (!fn || !world) return false;
		return fn(world, ray, start, end, filter, trace);
	}

	static bool make_filter(trace_filter_t* filter, void* skip_entity, std::uint64_t mask, std::uint8_t layer)
	{
		if (!filter) return false;

		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, std::int32_t, std::int32_t);
		static function_t fn = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", TRACE_FILTER_INIT));

		if (!fn) return false;

		fn(filter, skip_entity, mask, layer, TRACE_FILTER_TYPE);

		return true;
	}

	static bool make_movement_filter(trace_filter_t* filter, void* skip_entity, std::uint64_t mask, std::int32_t collision_group)
	{
		if (!filter) return false;

		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, std::int32_t);
		static function_t fn = []() -> function_t {
			auto match = utilities::pattern_scan(L"client.dll", "00 00 41 B8 00 01 00 00 E8 ? ? ? ? 4C 39 7F 38");
			return match ? reinterpret_cast<function_t>(utilities::resolve_rip(match + 8, 1, 5)) : nullptr;
		}();

		if (!fn) return false;

		fn(filter, skip_entity, mask, collision_group);

		return true;
	}

	bool trace_line(const vec3_t& start, const vec3_t& end, trace_filter_t* filter, game_trace_t* trace)
	{
		ray_t ray = { };
		vec3_t trace_start = start;
		vec3_t trace_end = end;

		return trace_shape(&ray, trace_start, trace_end, filter, trace);
	}

	bool trace_line(const vec3_t& start, const vec3_t& end, void* skip_entity, game_trace_t* trace, std::uint64_t mask = MASK_PLAYER_VISIBLE, std::uint8_t layer = TRACE_LAYER_DEFAULT)
	{
		trace_filter_t filter = { };

		if (!make_filter(&filter, skip_entity, mask, layer)) return false;

		return trace_line(start, end, &filter, trace);
	}

	static std::uint8_t* trace_hull_call_site()
	{
		static std::uint8_t* call_site = utilities::scan_function(L"client.dll", TRACE_HULL_CALL);

		return call_site;
	}

	static void* trace_hull_target()
	{
		static void* target = utilities::pattern_scan(L"client.dll", "48 89 74 24 18 55 57 41 54 41 55 41 56 48 8D AC 24 ? ? ? ? 48 81 EC A0 01 00 00 4C 8B B5");

		return target;
	}

	static bool make_player_movement_filter(trace_filter_t* filter, player_t* player)
	{
		if (!filter || !player) return false;
		using function_t = void(__fastcall*)(trace_filter_t*, void*, std::uint64_t, int);
		static function_t fn = []() -> function_t {
			auto match = utilities::pattern_scan(L"client.dll", "00 00 41 B8 00 01 00 00 E8 ? ? ? ? 4C 39 7F 38");
			return match ? reinterpret_cast<function_t>(utilities::resolve_rip(match + 8, 1, 5)) : nullptr;
		}();
		if (!fn) return false;
		const auto collision = schema_system::get_schema("C_BaseModelEntity", "m_Collision");
		const auto attributes = schema_system::get_schema("CCollisionProperty", "m_collisionAttribute");
		const auto interacts = schema_system::get_schema("VPhysicsCollisionAttribute_t", "m_nInteractsWith");
		if (!collision || !schema_system::has_schema("CCollisionProperty", "m_collisionAttribute")
			|| !schema_system::has_schema("VPhysicsCollisionAttribute_t", "m_nInteractsWith")) return false;
		auto mask = *reinterpret_cast<std::uint64_t*>(reinterpret_cast<std::uintptr_t>(player) + collision + attributes + interacts);
		if (player->flags() & 0x10u) mask |= 0x20ull;
		fn(filter, player, mask, 11);
		return true;
	}

	bool trace_hull_player(const vec3_t& start, const vec3_t& end, const vec3_t& hull_mins, const vec3_t& hull_maxs, trace_filter_t* filter, void* movement_services, game_trace_t* trace)
	{
		struct bbox_t { vec3_t mins; vec3_t maxs; };
		using function_t = void(__fastcall*)(void*, game_trace_t*, const vec3_t*, const vec3_t*, const bbox_t*, trace_filter_t*);

		void* target = trace_hull_target();

		if (!utilities::is_valid_pointer(target)) return false;
		if (!utilities::is_valid_pointer(movement_services)) return false;

		function_t fn = reinterpret_cast<function_t>(target);

		if (!trace || !filter) return false;
		const bbox_t bounds{ hull_mins, hull_maxs };
		game_trace_t result{};
		auto mutable_filter = *filter;
		fn(reinterpret_cast<std::uint8_t*>(movement_services) + 0x7B0, &result, &start, &end, &bounds, &mutable_filter);
		*trace = result;

		return true;
	}

	static bool is_ready() { return get_trace_shape() != nullptr; }

	private:
	static void* get_trace_shape()
	{
		static void* trace_shape_address = utilities::scan_function(L"client.dll", TRACE_SHAPE);
		return trace_shape_address;
	}
};
