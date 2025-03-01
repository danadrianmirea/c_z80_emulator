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

# Find SDL2 and its components
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

# Link SDL2 and its components
target_link_libraries(zx_emulator PRIVATE 
    SDL2::SDL2 
    SDL2_ttf::SDL2_ttf 
    SDL2_image::SDL2_image
)