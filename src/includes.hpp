#pragma once

#include "utilities/luasys/lua/lua.hpp"
#include "utilities/signatures.hpp"
#include "utilities/utilities.hpp"
#include "utilities/vfunc.hpp"
#include "utilities/imgui/imgui.h"
#include "utilities/imgui/imgui_internal.h"
#include "utilities/renderer/renderer.hpp"

#include "source2-sdk/globals.hpp"
#include "source2-sdk/sdk.hpp"

import econ_item_class;
import players_class;
import class_skeleton;
import entity_class;
import trace_class;
import types;
import class_user_cmd;
import viewsetup_class;
import weapons_class;
import global_vars;
import convars;
import math;

#include "source2-sdk/interfaces/interfaces.hpp"
#include "source2-sdk/interfaces/i_engine_client.hpp"
#include "source2-sdk/interfaces/i_client.hpp"
#include "source2-sdk/interfaces/i_csgo_input.hpp"
#include "source2-sdk/interfaces/i_entity_list.hpp"
#include "source2-sdk/interfaces/i_file_system.hpp"
#include "source2-sdk/interfaces/i_input_system.hpp"
#include "source2-sdk/interfaces/i_localize.hpp"
#include "source2-sdk/interfaces/i_netclient.hpp"
#include "source2-sdk/interfaces/i_prediction.hpp"
#include "source2-sdk/interfaces/i_schema_system.hpp"
#include "source2-sdk/interfaces/i_trace.hpp"
#include "source2-sdk/interfaces/visible.hpp"
#include "source2-sdk/sdl/sdl.hpp"
#include "hooks/hooks.hpp"

#include "features/config/binds.hpp"
#include "features/menu/menu.hpp"
#include "features/config/config.hpp"
