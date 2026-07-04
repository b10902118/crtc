cmake_minimum_required(VERSION 3.13)
project(CRLoader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set RPATH/RUNPATH relative to the executable ($ORIGIN and $ORIGIN/lib)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/lib:\$ORIGIN")


option(ENABLE_GRAPHICS "Enable --no-headless option" OFF)


# Define the standalone executable
set(CR_LOADER_SRCS
    src/loader.cpp
    src/jni.cpp
    src/hooks.cpp
    src/base64.cpp
)

set(CR_LOADER_LIBS
    dl
)

# -DCMAKE_BUILD_TYPE=Debug
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

if(ENABLE_GRAPHICS)
    add_compile_definitions(ENABLE_GRAPHICS)
    list(APPEND CR_LOADER_SRCS src/egl.cpp)
    list(APPEND CR_LOADER_LIBS EGL GLESv2)
else()
    add_library(GLESv2 SHARED mocks/mock_glesv2.cpp)
    set_target_properties(GLESv2 PROPERTIES
        SUFFIX ".so.2"
    )
endif()
	
add_executable(cr_loader ${CR_LOADER_SRCS})
target_include_directories(cr_loader PRIVATE src)
target_link_options(cr_loader PRIVATE "-Wl,--disable-new-dtags")
target_link_libraries(cr_loader ${CR_LOADER_LIBS})

# Build our custom mock Android shared libraries
add_library(android SHARED mocks/mock_android.cpp mocks/mock_stdio.c)
target_link_libraries(android dl)
add_library(log SHARED mocks/mock_log.cpp)
add_library(fmod SHARED mocks/mock_fmod.cpp)


