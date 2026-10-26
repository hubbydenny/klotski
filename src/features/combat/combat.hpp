#pragma once

import players_class;
import class_user_cmd;
import math;

namespace combat
{
	bool run_aimbot(controller_t* local_controller, user_cmd_t* cmd);
	void run_rcs();
	void update_standalone_rcs(const vec3_t& view_angles, const vec3_t& aim_punch, int amount, int rand_min, int rand_max, bool apply);
}
