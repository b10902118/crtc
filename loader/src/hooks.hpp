#pragma once

// JNICALL Calling Convention:
// On 32-bit x86 (i386), the JNI specification mandates the callee-cleanup
// calling convention (__stdcall) for JNI functions. This is compiled into
// `retn <N>` instructions (e.g. `retn 4`) to clean up arguments. Hooking
// these functions with the standard __cdecl convention results in stack pointer 
// misalignment (ESP) after caller adjustments, leading to a SEGV.
// On ARM, JNICALL resolves to nothing as ARM has a single unified calling convention (AAPCS).
#if defined(__i386__) || defined(_M_IX86)
#define JNICALL __attribute__((stdcall))
#else
#define JNICALL
#endif

bool setup_hooks();