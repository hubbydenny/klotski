#pragma once
import math;
import class_user_cmd;

namespace movement
{
	void finish_command(user_cmd_t* cmd);
	void auto_bounce(user_cmd_t* cmd);
	void visualize_auto_bounce();
	extern bool ab_active;
	void run_bhop(user_cmd_t* cmd);
	void ej(user_cmd_t* cmd);
	void mini_jump(user_cmd_t* cmd);
	bool ej_active();
	extern bool lj_active;
	extern bool mj_active;
	bool jumpbug_detected();
	void ladder_bug(user_cmd_t* cmd);
	void edge_bug(user_cmd_t* cmd);
	void visualize_eb();
	void jump_bug(user_cmd_t* cmd);
	bool jumpbug_active();
	bool edgebug_active();
	bool edgebug_silent_active();
	void long_jump(user_cmd_t* cmd);
	void autobounce(user_cmd_t* cmd);
}
struct landing_prediction
{
	bool valid{ false };
	float fraction{ -1.0f };
	vec3_t normal{};
};
[[nodiscard]] landing_prediction predict_landing();

