#pragma once

import class_user_cmd;
import class_user_cmd;
import players_class;
import players_class;
import players_class;
import entity_class;
import global_vars;

class cprediction_t;
class INetworkClient;

class ctx {
public:
    struct prediction_context {
        user_cmd_t* cmd{};
        cbase_user_cmd_pb_t* base{};
        player_t* player{};
        controller_t* controller{};
        player_movement_services_t* movement{};
        game_scene_node_t* scene_node{};
        cprediction_t* prediction{};
        INetworkClient* network{};
        global_vars_t* globals{};
    };

    prediction_context prediction{};
};

inline ctx g_ctx{};
