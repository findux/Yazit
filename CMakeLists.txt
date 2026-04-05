cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "D:/Uzay/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "vcpkg toolchain")

project(Yazit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# x64 zorunlu
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Sadece x64 (64-bit) build desteklenmektedir!")
endif()

# UTF-8 kaynak dosya kodlamasi (Turkce karakter destegi)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ── Paketler ─────────────────────────────────────────────────────────────────
find_package(SDL2       CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf   CONFIG REQUIRED)
find_package(SDL2_net   CONFIG REQUIRED)
find_package(GLEW       CONFIG REQUIRED)
find_package(imgui      CONFIG REQUIRED)
find_package(Lua                REQUIRED)   # lua via vcpkg

# ── Kaynak dosyalar ───────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/App.cpp
    src/EditorTab.cpp
    src/FindState.cpp
    src/FileDialogs.cpp
    src/LuaEngine.cpp
    src/imgui_impl_sdl2.cpp
    src/ImGuiColorTextEdit/TextEditor.cpp
    src/app.rc
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "YAZIT")

# ── Include yolları ───────────────────────────────────────────────────────────
target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    src/ImGuiColorTextEdit/
    ${LUA_INCLUDE_DIR}
)

# ── Bağlantı ─────────────────────────────────────────────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    # SDL2
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
    $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>
    $<IF:$<TARGET_EXISTS:SDL2_net::SDL2_net>,SDL2_net::SDL2_net,SDL2_net::SDL2_net-static>

    # OpenGL + GLEW
    GLEW::GLEW
    opengl32

    # ImGui
    imgui::imgui

    # Lua
    ${LUA_LIBRARIES}
)

# ── Kurulum (dist/ klasörü) ───────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# vcpkg'den gelen DLL'leri yanına kopyala
install(DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
    DESTINATION .
    FILES_MATCHING PATTERN "*.dll"
)

# İkon dosyasını da kopyala (NSIS kullanacak)
install(FILES src/app.ico DESTINATION .)
