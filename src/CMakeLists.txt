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
    memory.h
    zx_spectrum.h
)

# Add executable target
add_executable(zx_emulator ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(zx_emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries (SDL2)
find_package(SDL2 REQUIRED)
target_link_libraries(zx_emulator PRIVATE SDL2::SDL2 SDL2::SDL2_ttf SDL2::SDL2_image)