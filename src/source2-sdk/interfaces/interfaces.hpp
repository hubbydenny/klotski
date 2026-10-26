#pragma once

#include "i_client.hpp"
#include "i_csgo_input.hpp"
#include "i_engine_client.hpp"
#include "i_entity_list.hpp"
#include "i_file_system.hpp"
#include "i_input_system.hpp"
#include "i_localize.hpp"
#include "i_schema_system.hpp"
#include "i_trace.hpp"
#include "i_netclient.hpp"
#include "i_prediction.hpp"

import convars;
import global_vars;

namespace interfaces
{
	extern i_client* client;
	extern i_csgo_input* csgo_input;
	extern i_engine_client* engine;
	extern i_entity_list* entity_list;
	extern i_input_system* input_system;
	extern i_file_system* file_system;
	extern i_localize* localize;
	extern i_schema_system* schema_system;
	extern i_trace* trace;
	extern enginecvr* cvar;
	extern global_vars_t* globals;
	extern user_cmd_t* cmd;
	extern I_NetworkClientService* netclient;
	extern cprediction_t* i_pred;

	bool initialize();
}
