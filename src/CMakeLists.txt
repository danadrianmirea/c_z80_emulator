# List source files
set(SOURCES
    z80.c
    memory.c
    loader.c
    main.c
)

# List header files (optional, for IDE support)
set(HEADERS
    z80.h
    memory.h
    loader.h
)

# Add executable target
add_executable(zx_emulator ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(zx_emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set compiler flags for Release mode
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    # Check if the compiler is GCC or Clang
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Add -s flag to strip debug symbols in Release mode
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
    endif()
endif()

# Find and link SDL2 components
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

target_link_libraries(zx_emulator PRIVATE 
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf 
    SDL2_image::SDL2_image
)

# Post-build step: Copy executable to /bin
add_custom_command(TARGET zx_emulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:zx_emulator>
        ${CMAKE_SOURCE_DIR}/bin/zx_emulator.exe
)