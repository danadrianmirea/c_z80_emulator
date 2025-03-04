# List source files
set(SOURCES
    zx_spectrum.c
    main.c
)

# List header files (optional, for IDE support)
set(HEADERS
    zx_spectrum.h
)

# Add executable target
add_executable(zx_emulator ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(zx_emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Set compile options based on build type
target_compile_options(zx_emulator PRIVATE
    $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>
    $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}>
)

# Set flags for GCC or G++
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-Og -Wall -Wextra")
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