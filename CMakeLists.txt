cmake_minimum_required(VERSION 3.16)
project(TerraForge VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(BUILD_SHARED_LIBS OFF)

# --- SFML Download ---
include(FetchContent)
FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.x
)
FetchContent_MakeAvailable(sfml)

# --- EXPLICIT SOURCE LIST ---
# We list files manually to avoid duplication errors.
# Also, listing .h files helps CLion show them in the project tree.
add_executable(TerraForge
        src/main.cpp       # <--- Don't forget main.cpp!
        src/Game.cpp
        src/Game.h
        src/Player.cpp
        src/Player.h
        src/World.cpp
        src/World.h
        src/Dodo.h
        src/Dodo.cpp
)

# --- Linking ---
target_link_libraries(TerraForge PRIVATE sfml-graphics sfml-window sfml-system sfml-audio sfml-main)
# --- Assets Copy ---
add_custom_command(TARGET TerraForge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:TerraForge>/assets
)

# --- NUEVO: Copiar OpenAL automáticamente ---
add_custom_command(TARGET TerraForge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll
        $<TARGET_FILE_DIR:TerraForge>/openal32.dll
)