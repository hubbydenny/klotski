#pragma once
import convars;


import class_user_cmd;
class CConVar;
namespace misc {
	void run();
	void grenade_trail();
    void grenade_preview();
	void update_velocity();
	void practice_move(user_cmd_t* cmd);
	void practice_request(bool load);
	void grenade_pred(user_cmd_t* cmd);
}
