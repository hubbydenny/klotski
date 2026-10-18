#pragma once
#include <cstddef> // only for std::size_t | std::uintptr_t

// checkout cs2-sdk.com 

inline constexpr const char*  FRAME_STAGE_NOTIFY = "48 89 5C 24 ? 48 89 6C 24 ? 57 48 83 EC ? 48 8B F9 33 ED";
inline constexpr const char*  CREATE_MOVE = "85 D2 0F 85 ? ? ? ? 48 8B C4 44 88 40 18";
inline constexpr const char* PRESENT_PATTERN = "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 41 8B F0";
inline constexpr const char*  RESIZE_BUFFERS_PATTERN = "40 53 55 56 57 41 54 41 56 41 57 48 83 EC ? 44 8B E2";
inline constexpr const char*  MOUSE_INPUT_PATTERN = "40 53 48 83 EC 20 80 B9 ? ? ? ? ? 48 8B D9 75 78";
inline constexpr const char*  DRAW_GLOW_PATTERN = "40 53 48 83 EC 20 48 8B 54";
inline constexpr const char* FIND_HUD_ELEMENT_PATTERN "40 53 48 83 EC 20 48 8B 05 ? ? ? ? 48 8B D9 48 85 C0 74 ? 48 89 5C 24 38";
inline constexpr const char* HUD_VOICE_PRINT_CALL "E8 ? ? ? ? 4C 39 74 24 ? 74 ? 48 8D 4C 24 ? FF 15 ? ? ? ? 33 D2";// for better chatprint
inline constexpr const char* ENTITY_LIST = "48 8B 0D ? ? ? ? 8B D3 E8 ? ? ? ? 48 8B F0";
inline constexpr const char*  GET_BASE_ENTITY = "4C 8D 49 10 81 FA ? ? 00 00 77 ? 8B CA C1 F9 09";
inline constexpr const char* GET_LOCAL_PLAYER_CONTROLLER = "E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 44 8B 45 17 48 8B 38 4C 8B 55 0F 8B 75 0B 44 89 45 BB C7 45 B7 FF FF FF FF";

inline constexpr const char*  GLOW_MGR = "48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 8B 41";
inline constexpr const char*  IS_IN_GAME = "48 8B ? ? ? ? ? 48 85 C0 74 15 80 B8 ? ? ? ? ? 75 0C 83 B8 ? ? ? ? 06";
inline constexpr const char*  IS_CONNECTED = "48 8B 05 ? ? ? ? 48 85 C0 74 ? 83 B8 ? ? ? ? ? 0F 9D C0";

inline constexpr const char*  OVERRIDE_VIEW = "40 53 57 48 83 EC 58 48 8B FA E8";
inline constexpr const char*  SET_VIEW_ANGLES = "85 D2 75 ? 48 63 81";
inline constexpr const char*  GET_VIEW_ANGLES = "4C 8B C1 85 D2 74 ? 48 8D 05";
inline constexpr const char*  C_CSPlayerPawn_GetAimPunch = "48 89 5C 24 08 55 56 57 48 8D 6C 24 B9 48 81 EC";
inline constexpr const char*  GET_USER_CMD_TICK = "48 83 EC ? 4C 8B 0D ? ? ? ? 4C 8B DA";
inline constexpr const char*  GET_USER_CMD_ARRAY = "48 89 4C 24 ? 41 56 41 57";
inline constexpr const char*  GET_USER_CMD_BY_SEQUENCE_NUMBER = "40 53 48 83 EC ? 8B DA E8 ? ? ? ? 4C 8B C0";
inline constexpr const char*  FIRST_USER_CMD_ARRAY = "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF 4C 8B F8";

inline constexpr const char*  CLIP_TRACE_TO_PLAYERS = "48 8B C4 55 56 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 89 58";
inline constexpr const char* TRACE_SHAPE = "48 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 56 41 57 48 8D AC 24 ? ? ? ? B8";
inline constexpr const char*  TRACE_FILTER_INIT = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF 24";
inline constexpr const char*  INIT_PLAYER_MOVEMENT_TRACE_FILTER = "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 41 ? 33 FF C7 41 ?";
inline constexpr const char*  TRACE_HULL_CALL = "E8 ? ? ? ? 0F 2F 75 4C";
    
inline constexpr const char*  CSGO_INPUT = "48 8B 0D ? ? ? ? 48 8B 01 FF 50 ? 8B DF";
inline constexpr const char*  GLOBAL_VARS = "48 8B ? ? ? ? ? 8B 48 04 FF C1 89 8B 80 06 00 00 48 8B CB E8 ? ? ? ? F3 0F 10 ? ? ? ? ? 48 8D 4C 24 60 E8 ? ? ? ? 45 8B CE 48 8D ? ? ? ? ? 48 8B CB 44 8B 00";
inline constexpr const char*  TRACE_MANAGER = "48 8B 1D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 4C 8B 0B 4C 8D 44 24 ? 48 8B C8";
inline constexpr const char*  ALL_TYPE_SCOPE = "48 8B 05 ? ? ? ? 48 8B D6 0F B7 CB 48 8B 3C C8";
inline constexpr const char*  CALC_WORLD_SPACE_BONES = "48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 B8 98 42 00 00";
inline constexpr const char*  GET_BONE_ID_BY_NAME = "40 53 48 83 EC 20 48 8B 89 30 03 00 00 48 8B DA 48 8B 01 FF 50 68 48 8B C8";
inline constexpr const char*  WORLD_TO_SCREEN = "48 89 74 24 ? 57 48 83 EC ? 48 83 3D ? ? ? ? ? 48 8B FA";
inline constexpr const char*  CREATE_SUBTICK_MOVE_STEP = "48 8B 54 CA 08 8D 41 01 89 47 08 EB 16 48 8B 0F E8 ? ? ? ? 48 8B D0 48 8B CF";
inline constexpr const char* PROTOBUF_ADD_TO_REPEATED_PTR_ELEMENT = "E8 ? ? ? ? 4C 8B D0 45 8B 4A 10";
inline constexpr const char* SERIALIZE_MOVE_CRC = "48 89 5C 24 20 55 56 57 48 83 EC 30 49 8B C0";
inline constexpr const char*  GET_VDATA = "48 81 EC ? ? ? ? 48 85 C9 75 ? 33 C0 48 81 C4 ? ? ? ? C3";
inline constexpr const char*  GENERATE_PRIMITIVES = "48 8B C4 48 89 58 08 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 81 EC";
inline constexpr const char*  SCENE_ANIMATABLE_GENERATE_PRIMITIVES = "48 8B C4 48 89 58 20 4C 89 40 18 48 89 50 10 48 89 48 08";
inline constexpr const char* KV3_ALLOC = "40 53 48 83 EC 30 80 FA 06 0F B6 C2 41 B9 16";
inline constexpr const char*  KV3_DESTROY = "40 57 41 57 48 83 EC 38 4C 8B 01 44 8B FA 49 8B C0 48 8B F9 48 C1 E8 02";
inline constexpr const char* MATERIAL_CREATE = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 4C 8B F2";
inline constexpr const char*  SKYBOX_COLOR_PATTERN = "F2 0F 10 8F ? ? ? ? 4C 8D 44 24 50 F3 0F 10 9F ? ? ? ?";
inline constexpr const char* GET_VIEW_MODEL_OFFSETS = "40 55 53 56 41 56 41 57 48 8B EC 48 83 EC 20 4D 8B F8 4C 8B F2 48 8B F1 E8";
inline constexpr const char*  CALC_VIEWMODEL_VIEW = "40 53 48 83 EC 60 48 8B 41 08 49 8B D8 8B 48 30 48 C1 E9 0C F6 C1 01 0F 85 48 01 00 00 41 B8 07";
inline constexpr const char* FLASH = "85 D2 0F 88 ? ? ? ? 48 89 4C 24 08 55 56"; // poebota :D
inline constexpr const char* SETUP_FOID = "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 48 89 4C 24 08 57 41 54 41 55 41 56 41 57 48 83 EC 20 48 63 02";
inline constexpr const char* DRAW_SMOKE_VERTEX = "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B 9C 24 ? ? ? ? 4D 8B F8";
inline constexpr const char* CROSSHAIR_DRAW = "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 85";
//scalaformula
namespace patterns {
    inline constexpr const char* kRunScript = "4C 89 4C 24 ?? 4C 89 44 24 ?? 48 89 54 24 ?? 55 53 56 57 41 54 41 56 41 57 48 8D 6C 24";
    inline constexpr const char* kRunFrame = "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 81 EC 80 00 00 00 45 33 F6";

    inline constexpr std::size_t kRunFrameIndex = 5;
    inline constexpr std::size_t kRunScriptIndex = 77;
    inline constexpr std::size_t kMaxVTableSlots = 512;
    inline constexpr int kEngineGlobalOffset = 3;

    inline constexpr const char* kEngineGlobals[]{"4C 89 35 ?? ?? ?? ?? 48 8B 01", "4C 89 3D ?? ?? ?? ?? 41 8B F7", "48 8B 05 ?? ?? ?? ?? 48 8B 80 F8 0B 00 00 C3", };
    inline constexpr const char* kImageUrl = "40 53 57 41 56 48 81 EC 40 04 00 00";
}
