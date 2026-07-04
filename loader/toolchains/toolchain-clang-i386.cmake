# CMake Toolchain File for Linux x86 32-bit (i386) using Clang/LLVM
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/toolchain-clang-i386.cmake -B build_clang_i386 .

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i386)

# Use clang to unify cross-compile compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(TARGET_TRIPLE "i386-linux-gnu")
set(I386_FLAGS "-m32")

set(CMAKE_C_FLAGS_INIT "-target ${TARGET_TRIPLE} ${I386_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "-target ${TARGET_TRIPLE} ${I386_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
