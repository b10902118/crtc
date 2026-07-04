# CMake Toolchain File for Linux ARM 32-bit (armel) using Clang/LLVM
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=toolchains/toolchain-clang-armel.cmake -B build_clang_armel .

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Use clang to unify cross-compile compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(TARGET_TRIPLE "arm-linux-gnueabi")
set(ARM_FLAGS "-march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb")

set(CMAKE_C_FLAGS_INIT "-target ${TARGET_TRIPLE} ${ARM_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "-target ${TARGET_TRIPLE} ${ARM_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-target ${TARGET_TRIPLE}")
