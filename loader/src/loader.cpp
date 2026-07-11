#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits.h>
#include <new>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "base64.hpp"
#include "hooks.hpp"
#include "jni.hpp"
#include "log.hpp"
#include "offsets.hpp"
#include "payloads.hpp"

#ifdef ENABLE_GRAPHICS
#include "egl.hpp"
#endif

uintptr_t libg_base = 0;
static void *g_handle = nullptr;

jint width = 450;
jint height = 800;

std::string get_executable_directory() {
  Dl_info info;
  // Pass the address of the current function
  if (dladdr(reinterpret_cast<void *>(get_executable_directory), &info) != 0 &&
      info.dli_fname) {
    std::string path(info.dli_fname);
    size_t last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos) {
      return path.substr(0, last_slash);
    }
  }
  return ".";
}

bool load_libraries(const std::string &base_dir) {
  std::string g_path = base_dir + "/libg.so";

  LOGD("Loading game library libg.so from: ", g_path);
  g_handle = dlopen(g_path.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_DEEPBIND);
  if (!g_handle) {
    LOGE("Failed to load libg.so: ", dlerror());
    return false;
  }
  LOGD("libg.so loaded successfully at handle: ", g_handle);

  return true;
}

std::vector<unsigned char>
craftNpcMessagePayload(std::vector<unsigned char> deck_trainer_payload,
                       std::vector<unsigned char> deck_player_payload) {

  std::vector<unsigned char> res = {};
  res.insert(res.end(), NpcSectorStateMessage_part0,
             NpcSectorStateMessage_part0 + sizeof(NpcSectorStateMessage_part0));
  // somehow without login the trainer becomes lower and so the deck lower first
  res.insert(res.end(), deck_trainer_payload.begin(),
             deck_trainer_payload.end());
  res.insert(res.end(), NpcSectorStateMessage_part1,
             NpcSectorStateMessage_part1 + sizeof(NpcSectorStateMessage_part1));
  res.insert(res.end(), deck_player_payload.begin(), deck_player_payload.end());
  res.insert(res.end(), NpcSectorStateMessage_part2,
             NpcSectorStateMessage_part2 + sizeof(NpcSectorStateMessage_part2));
  return res;
}

bool is_game_loaded() {
  typedef int (*__is_game_initialized_t)(void *);
  __is_game_initialized_t __is_game_initialized =
      (__is_game_initialized_t)(libg_base + is_game_initialized_offset);
  void **pp_obj_53 = (void **)(libg_base + pp_obj_53_offset);
  if (!pp_obj_53)
    return false;
  void *p_obj_53 = *pp_obj_53;
  if (!p_obj_53)
    return false;
  return __is_game_initialized(p_obj_53) != 0;
}

std::vector<unsigned char> parse_hex(const std::string &hex) {
  std::vector<unsigned char> bytes;
  bytes.reserve(hex.length() / 2);
  auto hex_char_to_val = [](char c) -> int {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return 0;
  };
  for (size_t i = 0; i + 1 < hex.length(); i += 2) {
    int high = hex_char_to_val(hex[i]);
    int low = hex_char_to_val(hex[i + 1]);
    bytes.push_back((high << 4) | low);
  }
  return bytes;
}

void create_and_process_message(
    uintptr_t message_ctor_offset, int message_size,
    std::vector<unsigned char> payload,
    std::function<void(void *)> preprocess_message = nullptr) {
  typedef void *(*message_ctor_t)(void *);
  typedef int (*process_message_t)(void *, void *);
  typedef void (*sub_2650AC_t)(void *, int);
  typedef void *(*sub_265074_t)(void *);
  typedef void (*sub_260318_t)(void *, void *, uint32_t);
  process_message_t process_message =
      (process_message_t)(libg_base + process_message_offset);
  message_ctor_t message_ctor =
      (message_ctor_t)(libg_base + message_ctor_offset);
  sub_2650AC_t sub_2650AC = (sub_2650AC_t)(libg_base + sub_2650ac_offset);
  sub_265074_t sub_265074 = (sub_265074_t)(libg_base + sub_265074_offset);
  sub_260318_t sub_260318 = (sub_260318_t)(libg_base + sub_260318_offset);

  void **pp_struct6 = (void **)(libg_base + pp_struct6_offset);
  void **pp_message_manager = (void **)(libg_base + pp_message_manager_offset);

  auto create_message = [&]() -> void * {
    void *ptr = ::operator new(message_size);
    message_ctor(ptr);
    return ptr;
  };

  void *p_message = create_message();
  void *p_buf = ::operator new(payload.size());
  std::memcpy(p_buf, payload.data(), payload.size());
  sub_2650AC(p_message, 0);
  void *p_MsgPayload = sub_265074(p_message);
  sub_260318(p_MsgPayload, p_buf, payload.size());

  if (preprocess_message) {
    preprocess_message(p_message);
  }
  // process the message
  void *p_struct6 = *pp_struct6;
  int ret = process_message(p_struct6, p_message);

  ::operator delete(p_buf);
  ::operator delete(p_message);
}

void preprocess_login_ok_message(void *p_message) {
  void *account = ::operator new(8);
  *(void **)((uintptr_t)p_message + 0x30) = account;
  *(uint32_t *)account = 0xdeadbeee;
  *((uint32_t *)account + 1) = 0xdeadbeef;
}

void login(bool is_trainer) {
  void **pp_message_manager = (void **)(libg_base + pp_message_manager_offset);
  void *p_state = *pp_message_manager;
  if (is_trainer) {
    *(int *)p_state = 6; // logined, with account uninitialized
    return;
  }
  *(int *)p_state = 2; // connected

  std::vector<unsigned char> payload;
  payload.insert(payload.end(), loginOkMessage,
                 loginOkMessage + sizeof(loginOkMessage));

  create_and_process_message(login_ok_message_ctor_offset, 0x94, payload,
                             preprocess_login_ok_message);
}

void start_new_training_camp(std::vector<unsigned char> deck_trainer_payload,
                             std::vector<unsigned char> deck_player_payload) {
  auto payload =
      craftNpcMessagePayload(deck_trainer_payload, deck_player_payload);
  create_and_process_message(npc_sector_state_message_ctor_offset, 0x30,
                             payload);
}

std::string get_name(void *p_nameData) {
  if (!p_nameData)
    return "";
  int name_len = *(int *)p_nameData;
  if (name_len >= 8) {
    char *name_str = *(char **)((uintptr_t)p_nameData + 8);
    return name_str ? std::string(name_str) : "";
  } else {
    return std::string((char *)((uintptr_t)p_nameData + 8));
  }
}

int get_elixir_denominator(void *p_obj_102) {
  typedef int (*get_elixir_denominator_1B3470_t)(void *);
  get_elixir_denominator_1B3470_t get_elixir_denominator_1B3470 =
      (get_elixir_denominator_1B3470_t)(libg_base +
                                        get_elixir_denominator_offset);
  return get_elixir_denominator_1B3470(p_obj_102) * 10;
}

uintptr_t kings[2] = {0, 0};

struct GameObject {
  std::string name;
  int owner;
  int x, y, h;
  std::optional<double> heading_x, heading_y;
  std::optional<int> hp, shield;
  std::optional<std::vector<std::string>> buffs;
  std::optional<int> state, attack_duration, deploy_time;
};

struct GameState {
  int tick = 0;
  std::array<std::array<int, 2>, 2> elixirs = {{{0, 0}, {0, 0}}};
  int elixir_denominator = 280000; // 280000 normal, 140000 for double
  std::array<std::array<int, 4>, 2> hands = {{{0, 0, 0, 0}, {0, 0, 0, 0}}};
  std::array<int, 2> next_cards = {-1, -1};
  std::array<std::array<std::string, 8>, 2> decks;

  std::vector<GameObject> game_objects;
};

void *get_p_struct1() {
  void **pp_obj_53 = (void **)(libg_base + pp_obj_53_offset);
  if (!pp_obj_53 || !*pp_obj_53) {
    LOGE("get_p_struct1: p_obj_53 NULL");
    return nullptr;
  }
  void *p_obj_53 = *pp_obj_53;
  void *p_obj_56 = *(void **)((uintptr_t)p_obj_53 + 0x18);
  void *p_struct0 = *(void **)((uintptr_t)p_obj_56 + 0x54);
  if (!p_struct0) {
    LOGE("get_p_struct1: p_struct0 NULL");
    return nullptr;
  }
  void *p_struct1 = *(void **)((uintptr_t)p_struct0 + 0x24);
  if (!p_struct1) {
    LOGE("get_p_struct1: p_struct1 NULL");
    return nullptr;
  }
  return p_struct1;
}

GameState extract_game_state() {
  GameState game_state;
  void *p_struct1 = get_p_struct1();

  uint32_t len = *(uint32_t *)((uintptr_t)p_struct1 + 0x8);
  void **arr = *(void ***)p_struct1;
  if (!arr) {
    LOGE("extract_game_state: p_struct1.arr NULL");
    return game_state;
  }

  for (uint32_t i = 0; i < len; ++i) {
    void *p_obj = arr[i];
    if (!p_obj)
      continue;

    void *vptr = *(void **)p_obj;
    if (vptr == (void *)(libg_base + obj_98_vptr_offset)) {
      // it only quick about 20% (0.2s for c 3000 clean board), not much when
      // units interact.
      // Also moving pointer will error when reallocation happens.
      /*
       *units interact (uintptr_t *)p_struct1 += 4; (uint32_t
       **)((uintptr_t)p_struct1 + 0x8) -= 1;
       */
      continue; // obj_98_vptr
    }

    GameObject game_obj;

    int owner = *(int *)((uintptr_t)p_obj + 0x28);
    game_obj.owner = owner;

    void *p_obj_109 = *(void **)((uintptr_t)p_obj + 0x10);
    if (!p_obj_109) {
      LOGE("extract_game_state: p_obj_109 NULL");
    }
    void *p_nameData = *(void **)((uintptr_t)p_obj_109 + 4);
    game_obj.name = get_name(p_nameData);

    game_obj.x = *(int *)((uintptr_t)p_obj + 0x2c);
    game_obj.y = *(int *)((uintptr_t)p_obj + 0x30);
    game_obj.h = *(int *)((uintptr_t)p_obj + 0x3c);

    unsigned int compFlags = *(unsigned int *)((uintptr_t)p_obj + 0x44);

    if (compFlags & 1) {
      void *atkComp = *(void **)((uintptr_t)p_obj + 0x48);
      game_obj.attack_duration = *(int *)((uintptr_t)atkComp + 0x0c);
    }

    // Movement
    // FIXME: currently distinguish projectiles and area effect by hp component
    if (compFlags) {

      game_obj.state = *(int *)((uintptr_t)p_obj + 0x6c);
      game_obj.deploy_time = *(int *)((uintptr_t)p_obj + 0x9c);

      int heading_direction_x = *(int *)((uintptr_t)p_obj + 0x5c);
      int heading_direction_y = *(int *)((uintptr_t)p_obj + 0x60);
      // heading_direction_h? not 0x64 anyway
      double heading_x = heading_direction_x;
      double heading_y = heading_direction_y;
      // FIXME: original value seems to be 256 but x, y are rounded down
      // respectively, so heading direction is imprecise
      double normalize_factor =
          std::sqrt(heading_x * heading_x + heading_y * heading_y);
      if (normalize_factor > 0.0) {
        game_obj.heading_x = heading_x / normalize_factor;
        game_obj.heading_y = heading_y / normalize_factor;
      } else {
        game_obj.heading_x = 0.0;
        game_obj.heading_y = 0.0;
      }
    }

    if (compFlags & 4) {
      void *hpComp = *(void **)((uintptr_t)p_obj + 0x50);
      game_obj.hp = *(int *)((uintptr_t)hpComp + 0x8);
      game_obj.shield = *(int *)((uintptr_t)hpComp + 0x14);
    }

    if (compFlags & 8) {
      void *buffComp = *(void **)((uintptr_t)p_obj + 0x54);
      if (!buffComp) {
        LOGE("extract_game_state: buffComp NULL");
      }
      game_obj.buffs = std::vector<std::string>();
      int buff_len = *(int *)((uintptr_t)buffComp + 0x10);
      if (buff_len > 0) {
        void **buff_arr = *(void ***)((uintptr_t)buffComp + 0x8);
        if (buff_arr) {
          for (int j = 0; j < buff_len; ++j) {
            void *p_state = buff_arr[j];
            if (p_state) {
              void *p_spellData = *(void **)((uintptr_t)p_state + 0xc);
              if (p_spellData) {
                void *p_spellNameData = *(void **)((uintptr_t)p_spellData + 4);
                game_obj.buffs.value().push_back(get_name(p_spellNameData));
              }
            }
          }
        }
      }
    }

    game_state.game_objects.push_back(game_obj);

    // king tower additional processing
    if (vptr == (void *)(libg_base + king_tower_vptr_offset)) {
      // TODO only need once
      kings[owner] = (uintptr_t)p_obj;
      game_state.elixir_denominator = get_elixir_denominator(p_obj);
      game_state.tick = *(int *)((uintptr_t)p_obj + 0x148) * -1;

      if (owner >= 0 && owner < 2) {
        int *handcard_idxs = (int *)((uintptr_t)p_obj + 0xfc);
        for (int k = 0; k < 4; ++k) {
          game_state.hands[owner][k] = handcard_idxs[k];
        }

        int *p_next_card_idx = *(int **)((uintptr_t)p_obj + 0xd0);
        if (p_next_card_idx) {
          game_state.next_cards[owner] = *p_next_card_idx;
        }

        int elixir_whole = *(int *)((uintptr_t)p_obj + 0x12c);
        int elixir_frac = *(int *)((uintptr_t)p_obj + 0x130);
        game_state.elixirs[owner][0] = elixir_whole;
        game_state.elixirs[owner][1] = elixir_frac;
      }
    }
  }

  // extract deck from accountManager
  void *accountManager = *(void **)((uintptr_t)p_struct1 + 0x28);
  if (!accountManager) {
    LOGE("extract_game_state: accountManager is NULL");
  }

  void **p_decks = (void **)((uintptr_t)accountManager + 0x30);
  for (int n = 0; n < 2; ++n) {
    void *deck = p_decks[n];
    void **cards = (void **)deck;
    for (int i = 0; i < 8; ++i) {
      void *card = cards[i];
      void *summoner = *(void **)card;
      void *p_summonerNameData = *(void **)((uintptr_t)summoner + 4);
      game_state.decks[n][i] = get_name(p_summonerNameData);
    }
  }
  return game_state;
}

void *get_card(int king_idx, int card_idx) {
  typedef void *(*get_card_1B20D0_t)(void *, int);
  get_card_1B20D0_t get_card = (get_card_1B20D0_t)(libg_base + get_card_offset);
  return get_card((void *)kings[king_idx], card_idx);
}

void *get_deck_card(int king_idx, int deck_idx) {
  typedef void **(*get_deck_t)(void *);
  get_deck_t get_deck = (get_deck_t)(libg_base + get_deck_offset);
  void **deck = get_deck((void *)kings[king_idx]);
  return deck[deck_idx];
}

void summon_by_hand_idx(int king_idx, int card_idx, int x, int y) {
  typedef void (*summon_1B2190_t)(void *, void *, int, int, int, int);
  summon_1B2190_t summon = (summon_1B2190_t)(libg_base + summon_offset);
  void *card = get_card(king_idx, card_idx);
  if (card) {
    summon((void *)kings[king_idx], card, x, y, 0, 0);
  }
}

void summon_by_deck_idx(int king_idx, int deck_idx, int x, int y) {
  typedef void (*summon_1B2190_t)(void *, void *, int, int, int, int);
  summon_1B2190_t summon = (summon_1B2190_t)(libg_base + summon_offset);
  void *card = get_deck_card(king_idx, deck_idx);
  if (card) {
    summon((void *)kings[king_idx], card, x, y, 0, 0);
  }
}

void minimal_battle_update() {
  void **pp_obj_53 = (void **)(libg_base + pp_obj_53_offset);
  void *p_obj_53 = *pp_obj_53;
  void *p_obj_56 = *(void **)((uintptr_t)p_obj_53 + 0x18);
  typedef void (*tick_BAC64_t)(void *, float);
  tick_BAC64_t tick_BAC64 = (tick_BAC64_t)(libg_base + tick_offset);
  tick_BAC64(p_obj_56, 1.0 / 20);
}

bool update(bool headless) {
  if (headless) {
    minimal_battle_update();
    return false;
  } else {
    typedef jboolean (*GameApp_update_t)(JNIEnv_ptr, jclass_ptr);
    static GameApp_update_t GameApp_update =
        (GameApp_update_t)(libg_base + game_app_update_offset);
    return GameApp_update(mock_env, nullptr);
  }
}

bool init(bool headless, bool trainer_view) {
  LOGD("=== Starting Standalone Android C++ Loader ===");
  std::string base_dir = get_executable_directory();
  LOGD("libg.so dir: ", base_dir);

#ifdef ENABLE_GRAPHICS
  // Automatically configure library and driver search paths for Mesa EGL/GLES
  // (part of libg.so dependency)
  std::string dri_path = base_dir + "/lib";
  setenv("LIBGL_DRIVERS_PATH", dri_path.c_str(), 0);
  setenv("MESA_LOADER_DRIVER_PATH", dri_path.c_str(), 0);
#endif

  // Load libraries
  if (!load_libraries(base_dir)) {
    LOGE("Library loading sequence aborted due to error.");
    return false;
  }

  // cd to assets so libg can fopen assets
  std::string assets_dir = base_dir + "/assets";
  if (access(assets_dir.c_str(), F_OK) != 0) {
    LOGE("assets dir not found: ", assets_dir);
    return false;
  }
  chdir(assets_dir.c_str());

  // Initialize mock environment structures
  init_jni_mock();

  // Initialize headless EGL graphics context before loading / running the
  // engine
#ifdef ENABLE_GRAPHICS
  if (!headless && !init_headless_egl(width, height)) {
    LOGE("Headless EGL context setup failed. Exiting...");
    return false;
  }
#else
  if (!headless) {
    LOGE("Headless EGL/graphics support is disabled in this build.");
    return false;
  }
#endif

  typedef jint (*JNI_OnLoad_t)(JavaVM_ptr, void *);
  typedef jboolean (*GameApp_init_t)(JNIEnv_ptr, jclass_ptr, jint, jint,
                                     jstring_ptr);

  typedef jboolean (*GameApp_update_t)(JNIEnv_ptr, jclass_ptr);

  JNI_OnLoad_t jni_onload = (JNI_OnLoad_t)dlsym(g_handle, "JNI_OnLoad");

  if (!jni_onload) {
    LOGE("dlsym: Could not resolve JNI_OnLoad");
    return false;
  }

  Dl_info info;
  if (dladdr((void *)jni_onload, &info)) {
    libg_base = (uintptr_t)info.dli_fbase;
    LOGD("libg base: ", libg_base);
  } else {
    LOGE("dladdr: Failed to get libg base");
    return false;
  }

  GameApp_init_t game_init = (GameApp_init_t)(libg_base + game_app_init_offset);

  GameApp_update_t game_update =
      (GameApp_update_t)(libg_base + game_app_update_offset);

  // need libg_base initialized
  if (!setup_hooks()) {
    return false;
  }

  jni_onload(mock_vm, nullptr);

  jclass_ptr mock_class = (jclass_ptr)0xDEADBEEF; // Non-null dummy class handle
  jstring_ptr mock_string = nullptr;

  LOGD("Invoking GameApp_init(mock_env, class=", mock_class, ", w=", width,
       ", h=", height, ", str=null)");
  if (!game_init(mock_env, mock_class, width, height, mock_string)) {
    LOGE("GameApp_init returned false");
    return false;
  }

  LOGD("Invoking GameApp_update(mock_env, "
       "nullptr) until game loaded");
  while (!is_game_loaded()) {
    jboolean update_error = game_update(mock_env, nullptr);
    if (update_error) {
      LOGE("[ERROR] GameApp_update returned error");
      return false;
    }
  }
  login(trainer_view);
  LOGD("Game loaded. Ready to start training camp");
  return true;
}

std::string escape_json_string(const std::string &s) {
  std::string res;
  for (char c : s) {
    if (c == '"')
      res += "\\\"";
    else if (c == '\\')
      res += "\\\\";
    else if (c == '\n')
      res += "\\n";
    else if (c == '\r')
      res += "\\r";
    else if (c == '\t')
      res += "\\t";
    else
      res += c;
  }
  return res;
}

std::string to_json(int val) { return std::to_string(val); }

std::string to_json(double val) { return std::to_string(val); }

std::string to_json(const std::string &val) {
  return "\"" + escape_json_string(val) + "\"";
}

template <typename Container,
          typename = decltype(std::declval<Container>().begin()),
          typename = decltype(std::declval<Container>().end())>
std::string to_json(const Container &c) {
  std::string res = "[";
  bool first = true;
  for (const auto &item : c) {
    if (!first) {
      res += ",";
    }
    first = false;
    res += to_json(item);
  }
  res += "]";
  return res;
}

std::string game_object_to_json(const GameObject &obj) {
  std::string json = "{";
  json += "\"name\":\"" + escape_json_string(obj.name) + "\",";
  json += "\"owner\":" + std::to_string(obj.owner) + ",";
  json += "\"x\":" + std::to_string(obj.x) + ",";
  json += "\"y\":" + std::to_string(obj.y) + ",";
  json += "\"h\":" + std::to_string(obj.h);

  if (obj.heading_x.has_value()) {
    json += ",\"heading_x\":" + std::to_string(obj.heading_x.value());
  }
  if (obj.heading_y.has_value()) {
    json += ",\"heading_y\":" + std::to_string(obj.heading_y.value());
  }
  if (obj.hp.has_value()) {
    json += ",\"hp\":" + std::to_string(obj.hp.value());
  }
  if (obj.shield.has_value()) {
    json += ",\"shield\":" + std::to_string(obj.shield.value());
  }
  if (obj.state.has_value()) {
    json += ",\"state\":" + std::to_string(obj.state.value());
  }
  if (obj.attack_duration.has_value()) {
    json +=
        ",\"attack_duration\":" + std::to_string(obj.attack_duration.value());
  }
  if (obj.deploy_time.has_value()) {
    json += ",\"deploy_time\":" + std::to_string(obj.deploy_time.value());
  }
  if (obj.buffs.has_value()) {
    json += ",\"buffs\":[";
    for (size_t i = 0; i < obj.buffs.value().size(); ++i) {
      if (i > 0)
        json += ",";
      json += "\"" + escape_json_string(obj.buffs.value()[i]) + "\"";
    }
    json += "]";
  }
  json += "}";
  return json;
}

std::string to_json(const GameObject &obj) { return game_object_to_json(obj); }

std::string game_state_to_json(const GameState &state) {
  std::string json = "{";
  json += "\"tick\":" + std::to_string(state.tick) + ",";
  json += "\"elixirs\":" + to_json(state.elixirs) + ",";
  json += "\"elixir_denominator\":" + to_json(state.elixir_denominator) + ",";
  json += "\"hands\":" + to_json(state.hands) + ",";
  json += "\"next_cards\":" + to_json(state.next_cards) + ",";
  json += "\"decks\":" + to_json(state.decks) + ",";
  json += "\"game_objects\":" + to_json(state.game_objects);
  json += "}";
  return json;
}

int main(int argc, char **argv) {
  bool headless = true;
  bool trainer_view = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--no-headless") {
      headless = false;
    } else if (arg == "--trainer-view") {
      trainer_view = true;
    } else if (arg == "--display-dim" && i + 1 < argc) {
      std::string dim = argv[++i];
      size_t comma = dim.find(',');
      if (comma != std::string::npos) {
        try {
          width = std::stoi(dim.substr(0, comma));
          height = std::stoi(dim.substr(comma + 1));
        } catch (...) {
          LOGE("Invalid --display-dim format: ", dim);
        }
      }
    } else if (arg.rfind("--display-dim=", 0) == 0) {
      std::string dim = arg.substr(14);
      size_t comma = dim.find(',');
      if (comma != std::string::npos) {
        try {
          width = std::stoi(dim.substr(0, comma));
          height = std::stoi(dim.substr(comma + 1));
        } catch (...) {
          LOGE("Invalid --display-dim format: ", dim);
        }
      }
    }
  }

  if (!init(headless, trainer_view)) {
    LOGE("init() failed");
    std::cout << "{\"status\":\"init() failed\"}" << std::endl;
    return 1;
  }

  std::cout << "{\"status\":\"ready\"}" << std::endl;

  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream iss(line);
    std::string cmd;
    if (!(iss >> cmd))
      continue;

    if (cmd == "step") {
      int p0_card = -1, p0_x = 0, p0_y = 0;
      int p1_card = -1, p1_x = 0, p1_y = 0;
      if (iss >> p0_card >> p0_x >> p0_y >> p1_card >> p1_x >> p1_y) {
        if (p0_card >= 0 && p0_card < 4) {
          summon_by_hand_idx(0, p0_card, p0_x, p0_y);
        }
        if (p1_card >= 0 && p1_card < 4) {
          summon_by_hand_idx(1, p1_card, p1_x, p1_y);
        }
      }
      bool error = update(headless);
      if (error) {
        LOGE("GameApp_update returned error");
      }
      std::cout << game_state_to_json(extract_game_state()) << std::endl;
    } else if (cmd == "summon") { // for experiment only
      int king_idx = -1, deck_idx = -1, x = 0, y = 0;
      if (iss >> king_idx >> deck_idx >> x >> y) {
        if (king_idx >= 0 && king_idx < 2 && deck_idx >= 0 && deck_idx < 8) {
          summon_by_deck_idx(king_idx, deck_idx, x, y);
        }
      }
    } else if (cmd == "c") {
      int n = 0;
      if (iss >> n && n > 0) {
#ifdef ENABLE_GRAPHICS
        if (headless) {
          for (int i = 0; i < n; ++i) {
            minimal_battle_update();
          }
        } else {
          // for no-headless ensure last tick is rendered for screenshot
          for (int i = 0; i < n - 1; ++i) {
            minimal_battle_update();
          }
          update(headless);
        }
#else
        for (int i = 0; i < n; ++i) {
          minimal_battle_update();
        }
#endif
      }
      std::cout << game_state_to_json(extract_game_state()) << std::endl;
    } else if (cmd == "reset") {
      std::string trainer_hex, player_hex;
      if (iss >> trainer_hex >> player_hex) {
        start_new_training_camp(parse_hex(trainer_hex), parse_hex(player_hex));
      } else {
        LOGE("reset: missing trainer deck or player deck hex payload.");
      }
      std::cout << game_state_to_json(extract_game_state()) << std::endl;
    } else if (cmd == "screenshot") {
#ifdef ENABLE_GRAPHICS
      if (headless) {
        std::cout << "{\"error\":\"headless mode\"}" << std::endl;
        LOGE("screenshot failed");
      } else {
        auto rgb_pixels = get_screenshot_pixels_rgb(width, height);
        std::string b64 = base64_encode(rgb_pixels.data(), rgb_pixels.size());
        std::cout << "{\"status\":\"ok\",\"pixels\":\"" << b64 << "\"}"
                  << std::endl;
      }
#else
      std::cout << "{\"error\":\"headless graphics support disabled\"}"
                << std::endl;
      LOGE("screenshot failed: graphics support disabled");
#endif

    } else if (cmd == "exit") {
      break;
    } else if (cmd == "readFile") {
      std::string path;
      if (iss >> path) {
        // read the file in binary and send the whole payload back
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
          std::streampos size = file.tellg();
          std::vector<unsigned char> buffer(size);
          file.seekg(0, std::ios::beg);
          file.read(reinterpret_cast<char *>(buffer.data()), size);
          file.close();
          std::cout << "{\"status\":\"ok\",\"data\":\""
                    << base64_encode(buffer.data(), buffer.size()) << "\"}"
                    << std::endl;
        } else {
          std::cout << "{\"status\":\"error\",\"message\":\"file not found\"}"
                    << std::endl;
        }
      } else {
        std::cout << "{\"status\":\"error\",\"message\":\"no path provided\"}"
                  << std::endl;
      }
    } else {
      std::cerr << "Unknown command: " << cmd << std::endl;
    }
  }

  // Clean up

#ifdef ENABLE_GRAPHICS
  if (!headless) {
    cleanup_egl();
  }
#endif
  LOGD("Done.");
  _exit(0); // prevent libg destructor SEGV
}
