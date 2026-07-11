#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#include "hooks.hpp"
#include "log.hpp"
#include "offsets.hpp"

extern uintptr_t libg_base;

namespace {
class HookManager {
private:
  uintptr_t pool_base;
  size_t pool_size;
  size_t current_offset;

  void set_page_writable(uintptr_t address, size_t size, bool writable) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = address & ~(page_size - 1);
    int prot = writable ? (PROT_READ | PROT_WRITE | PROT_EXEC)
                        : (PROT_READ | PROT_EXEC);
    mprotect((void *)page_start, page_size * 2, prot);
  }

#if defined(__arm__)
  // Encodes and writes a standard 32-bit Thumb-2 BL instruction
  void write_thumb_bl(uintptr_t call_site, uintptr_t target_address) {
    // 1. Calculate the raw relative displacement.
    // Thumb PC is always 4 bytes ahead of the current instruction execution
    // site.
    int32_t displacement = (int32_t)(target_address - (call_site + 4));

    // 2. Extract standard components
    int32_t sign = (displacement >> 24) & 1;
    int32_t i1 = (displacement >> 23) & 1;
    int32_t i2 = (displacement >> 22) & 1;
    int32_t imm10 = (displacement >> 12) & 0x3FF;
    int32_t imm11 = (displacement >> 1) & 0x7FF;

    // 3. Apply ARM architecture J1/J2 bit-flipping transformations
    int32_t j1 = (!i1) ^ sign;
    int32_t j2 = (!i2) ^ sign;

    // 4. Assemble the two 16-bit halves cleanly
    uint16_t first_half = 0xF000 | (sign << 10) | imm10;
    uint16_t second_half = 0xF800 | (j1 << 13) | (j2 << 11) | imm11;

    // 5. Safely apply memory modifications
    uint16_t *patch_ptr = (uint16_t *)call_site;
    set_page_writable(call_site, 4, true);

    patch_ptr[0] = first_half;
    patch_ptr[1] = second_half;

    set_page_writable(call_site, 4, false);
    __builtin___clear_cache((char *)call_site, (char *)(call_site + 4));
  }
#else
  // Encodes and writes a standard 32-bit x86 relative CALL instruction
  void write_x86_call(uintptr_t call_site, uintptr_t target_address) {
    int32_t displacement = (int32_t)(target_address - (call_site + 5));
    uint8_t *patch_ptr = (uint8_t *)call_site;
    set_page_writable(call_site, 5, true);
    patch_ptr[0] = 0xE8; // CALL rel32
    *(int32_t *)(patch_ptr + 1) = displacement;
    set_page_writable(call_site, 5, false);
    __builtin___clear_cache((char *)call_site, (char *)(call_site + 5));
  }
#endif

public:
  HookManager(uintptr_t dead_func_base, size_t size)
      : pool_base(dead_func_base), pool_size(size), current_offset(0) {}

  // Main API to hook a target call site
  bool hook_call_site(uintptr_t call_site, uintptr_t loader_hook_address) {
#if defined(__arm__)
    // Each absolute jump slot takes 12 bytes
    if (current_offset + 12 > pool_size) {
      return false;
    }

    uintptr_t slot_address = pool_base + current_offset;

    // 1. Write the absolute jump in the internal pool slot
    uint16_t *patch_ptr = (uint16_t *)slot_address;
    set_page_writable(slot_address, 12, true);

    // 1. LDR.W R12, [PC, #4] (4-byte instruction)
    // Note: In Thumb mode, PC is current instruction address + 4.
    // This loads the 32-bit word stored 4 bytes past the end of the PC pointer.
    patch_ptr[0] = 0xF8DF;
    patch_ptr[1] = 0xC004;

    // 2. BX R12 (2-byte instruction)
    patch_ptr[2] = 0x4760;

    // 3. NOP (2-byte instruction to align literal pool)
    patch_ptr[3] = 0xBF00;

    *(uint32_t *)(patch_ptr + 4) = (uint32_t)(loader_hook_address | 1);

    set_page_writable(slot_address, 8, false);
    __builtin___clear_cache((char *)slot_address, (char *)(slot_address + 8));

    // 2. Point the original call site to this slot via relative BL
    write_thumb_bl(call_site, slot_address);

    current_offset += 12;
    return true;
#else
    // Each absolute jump slot takes 10 bytes on x86
    if (current_offset + 10 > pool_size) {
      return false;
    }

    uintptr_t slot_address = pool_base + current_offset;
    uint8_t *patch_ptr = (uint8_t *)slot_address;
    set_page_writable(slot_address, 10, true);

    // Instruction: jmp *[slot_address + 6] (opcode FF 25)
    patch_ptr[0] = 0xFF;
    patch_ptr[1] = 0x25;
    *(uint32_t *)(patch_ptr + 2) = (uint32_t)(slot_address + 6);

    // Literal: absolute address of our hook function
    *(uint32_t *)(patch_ptr + 6) = (uint32_t)loader_hook_address;

    set_page_writable(slot_address, 10, false);
    __builtin___clear_cache((char *)slot_address, (char *)(slot_address + 10));

    // Point the original call site to this slot via relative CALL
    write_x86_call(call_site, slot_address);

    current_offset += 10;
    return true;
#endif
  }

  // Main API to hook a function directly by overwriting its entry point
  // (Tail-call / direct branch)
  bool hook_function(uintptr_t func_start, uintptr_t loader_hook_address) {
#if defined(__arm__)
    // Each absolute jump slot takes 12 bytes
    if (current_offset + 12 > pool_size) {
      return false;
    }

    uintptr_t slot_address = pool_base + current_offset;

    // 1. Write the absolute jump in the internal pool slot
    uint16_t *patch_ptr = (uint16_t *)slot_address;
    set_page_writable(slot_address, 12, true);

    // 1. LDR.W R12, [PC, #4] (4-byte instruction)
    // Note: In Thumb mode, PC is current instruction address + 4.
    // This loads the 32-bit word stored 4 bytes past the end of the PC pointer.
    patch_ptr[0] = 0xF8DF;
    patch_ptr[1] = 0xC004;

    // 2. BX R12 (2-byte instruction)
    patch_ptr[2] = 0x4760;

    // 3. NOP (2-byte instruction to align literal pool)
    patch_ptr[3] = 0xBF00;

    *(uint32_t *)(patch_ptr + 4) = (uint32_t)(loader_hook_address | 1);

    set_page_writable(slot_address, 12, false);
    __builtin___clear_cache((char *)slot_address, (char *)(slot_address + 12));

    // 2. Point the original function start to this slot via relative B.W branch
    uintptr_t target_func = func_start & ~1;

    // Calculate relative displacement for B.W. PC is target_func + 4.
    int32_t displacement = (int32_t)(slot_address - (target_func + 4));

    // Extract standard components
    int32_t sign = (displacement >> 24) & 1;
    int32_t i1 = (displacement >> 23) & 1;
    int32_t i2 = (displacement >> 22) & 1;
    int32_t imm10 = (displacement >> 12) & 0x3FF;
    int32_t imm11 = (displacement >> 1) & 0x7FF;

    // Apply ARM architecture J1/J2 bit-flipping transformations
    int32_t j1 = (!i1) ^ sign;
    int32_t j2 = (!i2) ^ sign;

    // Assemble the two 16-bit halves cleanly for B.W instruction
    uint16_t first_half = 0xF000 | (sign << 10) | imm10;
    uint16_t second_half = 0x9000 | (j1 << 13) | (j2 << 11) | imm11;

    uint16_t *func_ptr = (uint16_t *)target_func;
    set_page_writable(target_func, 4, true);

    func_ptr[0] = first_half;
    func_ptr[1] = second_half;

    set_page_writable(target_func, 4, false);
    __builtin___clear_cache((char *)target_func, (char *)(target_func + 4));

    current_offset += 12;
    return true;
#else
    // Each absolute jump slot takes 10 bytes on x86
    if (current_offset + 10 > pool_size) {
      return false;
    }

    uintptr_t slot_address = pool_base + current_offset;
    uint8_t *patch_ptr = (uint8_t *)slot_address;
    set_page_writable(slot_address, 10, true);

    // Instruction: jmp *[slot_address + 6] (opcode FF 25)
    patch_ptr[0] = 0xFF;
    patch_ptr[1] = 0x25;
    *(uint32_t *)(patch_ptr + 2) = (uint32_t)(slot_address + 6);

    // Literal: absolute address of our hook function
    *(uint32_t *)(patch_ptr + 6) = (uint32_t)loader_hook_address;

    set_page_writable(slot_address, 10, false);
    __builtin___clear_cache((char *)slot_address, (char *)(slot_address + 10));

    // Overwrite the start of the function with JMP rel32 to slot (5 bytes)
    int32_t displacement = (int32_t)(slot_address - (func_start + 5));
    uint8_t *func_ptr = (uint8_t *)func_start;
    set_page_writable(func_start, 5, true);
    func_ptr[0] = 0xE9; // JMP rel32
    *(int32_t *)(func_ptr + 1) = displacement;
    set_page_writable(func_start, 5, false);
    __builtin___clear_cache((char *)func_start, (char *)(func_start + 5));

    current_offset += 10;
    return true;
#endif
  }

  // Overwrites the specified address with NOP instructions
  bool patch_nop(uintptr_t address, size_t size) {
#if defined(__arm__)
    uintptr_t target_addr = address & ~1;
    set_page_writable(target_addr, size, true);
    uint16_t *ptr = (uint16_t *)target_addr;
    size_t count = size / 2;
    for (size_t i = 0; i < count; ++i) {
      ptr[i] = 0xBF00; // 16-bit Thumb NOP
    }
    set_page_writable(target_addr, size, false);
    __builtin___clear_cache((char *)target_addr, (char *)(target_addr + size));
#else
    set_page_writable(address, size, true);
    uint8_t *ptr = (uint8_t *)address;
    for (size_t i = 0; i < size; ++i) {
      ptr[i] = 0x90; // x86 NOP
    }
    set_page_writable(address, size, false);
    __builtin___clear_cache((char *)address, (char *)(address + size));
#endif
    return true;
  }
};

enum HookType { HOOK_CALL_SITE, HOOK_FUNCTION, HOOK_NOP };

struct HookTarget {
  HookType type;
  uintptr_t offset;
  uintptr_t value; // Destination address for hooks, or size/byte count for NOPs
};

// hooks
void hooked_sub_1B2EAC(void *player, int a2, int a3, void *allowed_region,
                       void *logic_summoner) {
  typedef void (*sub_1B2EAC_t)(void *player, int a2, int a3,
                               void *allowed_region, void *logic_summoner);
  sub_1B2EAC_t sub_1B2EAC =
      (sub_1B2EAC_t)(libg_base + summon_boundry_func_offset);
  sub_1B2EAC(player, a2, a3, allowed_region, logic_summoner);
  if (((int *)allowed_region)[1] == 31000)
    ((int *)allowed_region)[1] = 32000;
}

void hooked_tick_call(void *p_obj_56, float delay) {
  typedef void (*tick_BAC64_t)(void *, float);
  tick_BAC64_t tick_BAC64 = (tick_BAC64_t)(libg_base + tick_offset);
  tick_BAC64(p_obj_56, 1.0 / 20);
}

bool hooked_is_in_sc_splash_screen(void *obj) { return false; }

struct ScString {
  int len;
  int capacity;
  char data[8];
  int unk_words;
};

// JNI function: requires JNICALL (__stdcall on x86) to prevent SEGV due to
// callee-cleanup
void JNICALL hooked_get_version(struct ScString *s) {
  s->len = 5;
  s->capacity = 5;
  s->data[0] = '1';
  s->data[1] = '.';
  s->data[2] = '9';
  s->data[3] = '.';
  s->data[4] = '2';
  s->data[5] = '\0';
  s->unk_words = 0;
}

std::vector<HookTarget> hook_registry = {
    {HOOK_CALL_SITE, summon_boundry_call_offset, (uintptr_t)&hooked_sub_1B2EAC},
    {HOOK_CALL_SITE, tick_call_offset, (uintptr_t)&hooked_tick_call},
    {HOOK_FUNCTION, is_in_sc_splash_screen,
     (uintptr_t)&hooked_is_in_sc_splash_screen},
    {HOOK_FUNCTION, get_version_start_offset, (uintptr_t)&hooked_get_version},
    {HOOK_NOP, nop_do_message_call_offset, call_size},
    {HOOK_NOP, nop_nanosleep_call_offset, call_size},
    {HOOK_NOP, nop_ai_logic_call_offset, call_size},
    {HOOK_NOP, nop_login_segv_offset, call_size}};
} // namespace

bool setup_hooks() {
  // Automatically process and scale every hook in the registry
  const uintptr_t do_messaging_start = libg_base + do_messaging_offset;
  const uintptr_t do_messaging_end = libg_base + do_messaging_end_offset;
  HookManager manager(do_messaging_start,
                      do_messaging_end - do_messaging_start);
  for (const auto &hook : hook_registry) {
    uintptr_t target_abs = libg_base + hook.offset;
    switch (hook.type) {
    case HOOK_CALL_SITE:
      if (!manager.hook_call_site(target_abs, hook.value)) {
        LOGE("Failed to install hook for call-site: 0x%X", hook.offset);
      }
      break;
    case HOOK_FUNCTION:
      if (!manager.hook_function(target_abs, hook.value)) {
        LOGE("Failed to install hook for function: 0x%X", hook.offset);
      }
      break;
    case HOOK_NOP:
      if (!manager.patch_nop(target_abs, hook.value)) {
        LOGE("Failed to install NOP patch for offset: 0x%X", hook.offset);
      }
      break;
    }
  }

  return true;
}