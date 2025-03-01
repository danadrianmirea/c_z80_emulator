# src/CMakeLists.txt

# List source files
set(SOURCES
    cpu.c
    memory.c
    main.c
)

# List header files (optional, for IDE support)
set(HEADERS
    cpu.h
    zx_spectrum.h
)

# Add executable target
add_executable(zx_emulator ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(zx_emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

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