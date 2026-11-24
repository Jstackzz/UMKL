# MKArcade.exe: the menus (QML), the game thread, controllers and sound (SDL3) and rollback netplay (GekkoNet).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Gui Network)
qt_standard_project_setup(REQUIRES 6.5)

# SDL3 (local copy, static): controllers and sound only, Qt draws everything
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../external/SDL3" "${CMAKE_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)

# GekkoNet (local copy, static, with its asio UDP adapter). Its CMake picks the static C runtime; Qt uses the DLL one.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../external/GekkoNet/GekkoLib" "${CMAKE_BINARY_DIR}/gekkonet"
                 EXCLUDE_FROM_ALL)
set_target_properties(GekkoNet PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gekkonet")
target_compile_definitions(GekkoNet PRIVATE _WIN32_WINNT=0x0A00)

qt_add_executable(MKArcade WIN32
    src/main.cpp
    src/EmuCore.cpp src/EmuCore.h
    src/GameSession.cpp src/GameSession.h
    src/GameView.cpp src/GameView.h
    src/InputHub.cpp src/InputHub.h
    src/Library.cpp src/Library.h
    src/Settings.cpp src/Settings.h
    # online play on the MK3D server: its launcher's client (ServerClient, MatchController and the UDP parts), the
    # stand-ins for that launcher's own files, and the hub the menus see
    src/online/AppPaths.cpp src/online/AppPaths.h
    src/online/FeedLink.cpp src/online/FeedLink.h
    src/online/GameData.cpp src/online/GameData.h src/online/Style.h
    src/online/GameProcess.cpp src/online/GameProcess.h
    src/online/MatchController.cpp src/online/MatchController.h
    src/online/OnlineHub.cpp src/online/OnlineHub.h
    src/online/ServerClient.cpp src/online/ServerClient.h
    src/online/TunnelBridge.cpp src/online/TunnelBridge.h
    src/online/UdpRendezvous.cpp src/online/UdpRendezvous.h
    src/online/WinUtil.cpp src/online/WinUtil.h)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(MKArcade
    URI MKArcade
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Background.qml
        qml/Backdrop.qml
        qml/ButtonGlyph.qml
        qml/PromptBar.qml
        qml/GoldText.qml
        qml/GameCard.qml
        qml/ModeButton.qml
        qml/Tile.qml
        qml/SettingRow.qml
        qml/Toast.qml
        qml/OnScreenKeyboard.qml
        qml/TitleScreen.qml
        qml/HomeScreen.qml
        qml/OnlineScreen.qml
        qml/DirectScreen.qml
        qml/SettingsScreen.qml
        qml/GameScreen.qml
        qml/AccountChip.qml
        qml/MatchFound.qml
        qml/SignInScreen.qml
        qml/MatchScreen.qml
        qml/LobbiesScreen.qml
        qml/LobbyRoomScreen.qml
        qml/LeaderboardScreen.qml)

target_link_libraries(MKArcade PRIVATE
    Qt6::Quick Qt6::QuickControls2 Qt6::Gui Qt6::Network
    SDL3::SDL3-static GekkoNet winmm)
target_compile_definitions(MKArcade PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN MKA_VERSION="${PROJECT_VERSION}")

# the emulation core goes beside the executable
add_dependencies(MKArcade mka_fbneo)
add_custom_command(TARGET MKArcade POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:mka_fbneo>" "$<TARGET_FILE_DIR:MKArcade>")
