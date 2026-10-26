module;

#include <cstddef>
#include <cstdint>

export module trace_class;
export import math;
export import players_class;

export {
typedef struct _ray_t
{
	vec3_t mins;
	vec3_t maxs;
	char pad1[0x10];
	std::uint8_t type;
	char pad2[0x7];
} ray_t;

static_assert(offsetof(ray_t, type) == 0x28, "ray_t has wrong type offset");
static_assert(sizeof(ray_t) == 0x30, "ray_t has wrong size");

class hitbox_t {
public:
	const char* name;
	const char* surface_property;
	const char* bone_name;
	vec3_t mins;
	vec3_t maxs;
	float shape_radius;
	std::uint32_t bone_name_hash;
	std::int32_t group_id;
	std::uint8_t shape_type;
	bool translation_only;
	char pad1[0x2];
	std::uint32_t crc;
	std::uint32_t collision_attribute_index;
	std::uint16_t hitbox_index;
	char pad2[0x6];
};

static_assert(sizeof(hitbox_t) == 0x50, "hitbox_t has wrong size");

typedef struct _trace_filter_t
{
	std::byte data[0x48];
} trace_filter_t;

static_assert(sizeof(trace_filter_t) == 0x48, "trace_filter_t has wrong size");

typedef struct _game_trace_t {
	void* surface;
	player_t* player;
	hitbox_t* hitbox;
	char pad1[0x38];
	std::uint32_t surface_flags;
	char pad2[0x24];
	vec3_t startpos;
	vec3_t endpos;
	vec3_t normal;
	char pad3[0xC];
	char pad4[0x4];
	float fraction;
	char pad5[0x6];
	bool all_solid;
	char pad6[0x4];
	bool start_solid;
	char pad7[0x48];
} game_trace_t;

static_assert(offsetof(game_trace_t, fraction) == 0xAC, "game_trace_t has wrong fraction offset");
static_assert(offsetof(game_trace_t, all_solid) == 0xB6, "game_trace_t has wrong all_solid offset");
static_assert(offsetof(game_trace_t, start_solid) == 0xBB, "game_trace_t has wrong start_solid offset");
static_assert(sizeof(game_trace_t) == 0x108, "game_trace_t has wrong size");
}
