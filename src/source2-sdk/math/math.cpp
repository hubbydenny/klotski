module;

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../utilities/utilities.hpp"
#include "../sdk.hpp"

module math;

vec2_t::vec2_t()
    : x(0.f), y(0.f)
{
}

vec2_t::vec2_t(float x, float y)
    : x(x), y(y)
{
}

vec2_t vec2_t::operator+(const vec2_t& other) const
{
    return { x + other.x, y + other.y };
}


vec3_t::vec3_t()
    : x(0.f), y(0.f), z(0.f)
{
}

vec3_t::vec3_t(float x, float y, float z)
    : x(x), y(y), z(z)
{
}

vec3_t vec3_t::operator+(const vec3_t& other) const
{
    return {
        x + other.x,
        y + other.y,
        z + other.z
    };
}

vec3_t vec3_t::operator-(const vec3_t& other) const
{
    return {
        x - other.x,
        y - other.y,
        z - other.z
    };
}

vec3_t& vec3_t::operator+=(const vec3_t& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

vec3_t vec3_t::operator*(float scale) const
{
    return {
        x * scale,
        y * scale,
        z * scale
    };
}

void vec3_t::clamp()
{
    x = (std::clamp)(x, -89.f, 89.f);
    y = (std::clamp)(std::remainder(y, 360.f), -180.f, 180.f);
    z = (std::clamp)(z, -50.f, 50.f);
}

bool vec3_t::is_zero() const
{
    return x == 0.f && y == 0.f && z == 0.f;
}


vec4_t::vec4_t()
    : x(0.f), y(0.f), z(0.f), w(0.f)
{
}

vec4_t::vec4_t(float x, float y, float z, float w)
    : x(x), y(y), z(z), w(w)
{
}


color_t::color_t()
    : r(0), g(0), b(0), a(0)
{
}

color_t::color_t(std::uint8_t red,std::uint8_t green,std::uint8_t blue,std::uint8_t alpha): r(red), g(green), b(blue), a(alpha) {}

std::uint32_t color_t::dump() const
{
    return  static_cast<std::uint32_t>(r)| (static_cast<std::uint32_t>(g) << 8)| (static_cast<std::uint32_t>(b) << 16)| (static_cast<std::uint32_t>(a) << 24);
}

color_t color_t::black(std::uint8_t a)
{
    return { 0, 0, 0, a };
}

color_t color_t::white(std::uint8_t a)
{
    return { 255, 255, 255, a };
}

color_t color_t::red(std::uint8_t a)
{
    return { 255, 0, 0, a };
}

color_t color_t::green(std::uint8_t a)
{
    return { 0, 255, 0, a };
}

color_t color_t::blue(std::uint8_t a)
{
    return { 0, 0, 255, a };
}

color_t color_t::yellow(std::uint8_t a)
{
    return { 247, 202, 24, a };
}


float normalize_angle(float angle) {
    angle = std::fmod(angle, 360.f);

    if (angle > 180.f) angle -= 360.f;
    else if (angle < -180.f)angle += 360.f;
    return angle;
}

namespace math
{
    bool world_to_screen(vec3_t& origin, vec3_t& screen)
    {
        using function_t = bool(__fastcall*)(vec3_t&, vec3_t&);
        static function_t function = reinterpret_cast<function_t>(utilities::scan_function(L"client.dll", WORLD_TO_SCREEN));
        if (!function) return false;

        const bool status = !function(origin, screen);
        screen.x = static_cast<float>((screen.x + 1.0) * 0.5) * sdk::screen_width;
        screen.y = sdk::screen_height - static_cast<float>((screen.y + 1.0) * 0.5) * sdk::screen_height;
        return status;
    }

    vec3_t calculate_angle(
        const vec3_t& source,
        const vec3_t& destination,
        const vec3_t& view_angles)
    {
        const vec3_t trace(
            destination.x - source.x,
            destination.y - source.y,
            destination.z - source.z);

        const float flat_distance = std::hypot(trace.x, trace.y);

        vec3_t angles;

        if (flat_distance == 0.f)
        {
            angles.x = view_angles.x;
            angles.y = view_angles.y;
        }
        else
        {
            constexpr float rad_to_deg = 180.f / std::numbers::pi_v<float>;

            angles.x = std::atan2(-trace.z, flat_distance) * rad_to_deg;
            angles.y = std::atan2(trace.y, trace.x) * rad_to_deg;
        }

        angles.z = 0.f;

        angles.x = normalize_angle(angles.x - view_angles.x);
        angles.y = normalize_angle(angles.y - view_angles.y);

        return angles;
    }

    float angle_distance(
        const vec3_t& source,
        const vec3_t& destination,
        const vec3_t& view_angles)
    {
        const vec3_t delta = calculate_angle(source, destination, view_angles);
        return std::hypot(delta.x, delta.y);
    }
}

float normalize_yaw(float yaw)
{
    yaw = std::fmod(yaw + 180.f, 360.f);

    if (yaw < 0.f) yaw += 360.f;
    return yaw - 180.f;
}

float vel2d(const vec3_t& velocity)
{
    return std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
}
