#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

import class_user_cmd;

namespace prediction
{
	constexpr std::size_t max_simulation_ticks = 128;
	struct prestate_t
	{
		vec3_t networked_origin{};
		vec3_t networked_velocity{};
		std::uint32_t flags{};
		bool valid{};
	};

	void capture_prestate();
	const prestate_t& pre();

	class simulation_t
	{
	public:
		explicit simulation_t(user_cmd_t* cmd, std::size_t ticks);
		~simulation_t();

		simulation_t(const simulation_t&) = delete;
		simulation_t& operator=(const simulation_t&) = delete;

		bool valid() const;
		bool prepare_tick();
		bool run_tick();
		std::size_t tick() const;

	private:
		struct impl_t;
		std::unique_ptr<impl_t> impl;
	};

	void start(user_cmd_t* cmd);
	void end(user_cmd_t* cmd);
}
