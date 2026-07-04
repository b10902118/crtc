#pragma once

#include <cstdint>

// Offsets added to libg_base in wrapper.cpp

#if defined(__arm__)
const uintptr_t game_app_init_offset = 0x20F774 | 1;
const uintptr_t game_app_update_offset = 0x20FCA8 | 1;
const uintptr_t is_game_initialized_offset = 0xbc5ec | 1;
const uintptr_t pp_obj_53_offset = 0x59d980;
const uintptr_t process_message_offset = 0x996dc | 1;
const uintptr_t npc_sector_state_message_ctor_offset = 0x1f17d8 | 1;
const uintptr_t login_ok_message_ctor_offset = 0x1E5374 | 1;
// const uintptr_t operator_new_offset = 0x5a44c;
const uintptr_t sub_2650ac_offset = 0x2650ac | 1;
const uintptr_t sub_265074_offset = 0x265074 | 1;
const uintptr_t sub_260318_offset = 0x260318 | 1;
const uintptr_t pp_struct6_offset = 0x59ca60;
const uintptr_t pp_message_manager_offset = 0x59cbf0;
const uintptr_t get_elixir_denominator_offset = 0x1b3470 | 1;
const uintptr_t obj_98_vptr_offset = 0x50dd30;
const uintptr_t king_tower_vptr_offset = 0x50dee0;
const uintptr_t get_card_offset = 0x1b20d0 | 1;
const uintptr_t get_deck_offset = 0x1B20F4 | 1;
const uintptr_t summon_offset = 0x1b2190 | 1;
const uintptr_t tick_offset = 0xbac64 | 1;

const uintptr_t do_messaging_offset = 0x9E614;
const uintptr_t do_messaging_end_offset = 0x9EC62;
const uintptr_t summon_boundry_call_offset = 0x01B2698;
const uintptr_t summon_boundry_func_offset = 0x1B2EAC | 1;
const uintptr_t tick_call_offset = 0xBA98C;
const uintptr_t get_version_start_offset = 0x202658;

const unsigned long call_size = 4;
const uintptr_t nop_do_message_call_offset = 0x6B742;
const uintptr_t nop_nanosleep_call_offset = 0x20FD8A;
const uintptr_t nop_ai_logic_call_offset = 0x1B330E;
const uintptr_t nop_login_segv_offset = 0x9B036;

#else // i386
const uintptr_t game_app_init_offset = 0x304C50;
const uintptr_t game_app_update_offset = 0x3053F0;
const uintptr_t is_game_initialized_offset = 0xEC84A;
const uintptr_t pp_obj_53_offset = 0x8AE920;
const uintptr_t process_message_offset = 0xB9034;
const uintptr_t npc_sector_state_message_ctor_offset = 0x2CF4DC;
const uintptr_t login_ok_message_ctor_offset = 0x2B66A8;
// const uintptr_t operator_new_offset = ...;
const uintptr_t sub_2650ac_offset = 0x3B3230;
const uintptr_t sub_265074_offset = 0x3B31B0;
const uintptr_t sub_260318_offset = 0x3AA490;
const uintptr_t pp_struct6_offset = 0x8ADA3C;
const uintptr_t pp_message_manager_offset = 0x8ADBBC;
const uintptr_t get_elixir_denominator_offset = 0x262F8C;
const uintptr_t obj_98_vptr_offset = 0x81ED10;
const uintptr_t king_tower_vptr_offset = 0x81EEC0;
const uintptr_t get_card_offset = 0x260E90;
const uintptr_t get_deck_offset = 0x260ED8;
const uintptr_t summon_offset = 0x260FEE;
const uintptr_t tick_offset = 0xEA14C;

const uintptr_t do_messaging_offset = 0xC00C6;
const uintptr_t do_messaging_end_offset = 0xC0A2B;
const uintptr_t summon_boundry_call_offset = 0x261979;
const uintptr_t summon_boundry_func_offset = 0x262706;
const uintptr_t tick_call_offset = 0xE9DEB;
const uintptr_t get_version_start_offset = 0x2EF8D0;

const unsigned long call_size = 5;
const uintptr_t nop_do_message_call_offset = 0x7330B;
const uintptr_t nop_nanosleep_call_offset = 0x30555B;
const uintptr_t nop_ai_logic_call_offset = 0x262D62;
const uintptr_t nop_login_segv_offset = 0xBB18E;

#endif