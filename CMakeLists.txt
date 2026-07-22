cmake_minimum_required(VERSION 3.28)

project(GameVerseArena LANGUAGES CXX)

include(FetchContent)

set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

add_executable(GameVerseArena
    "5x5 Tic Tac Toe.cpp"
    "Diamond.cpp"
    "FourInRow.cpp"
    "Infinity_XO.cpp"
    "Memory_XO.cpp"
    "Misere_Board.cpp"
    "NumericalTicTacToe.cpp"
    "Obstacle.cpp"
    "pyramid.cpp"
    "SUS.cpp"
    "SUS_AI.cpp"
    "T4x4_Classes.cpp"
    "Ultimate_TTT.cpp"
    "Ultimate_TTT_AI.cpp"
    "Word_Tic_Tac_Toe.cpp"
    "XO_Classes.cpp"
    "XO_Demo.cpp"
)

set_target_properties(GameVerseArena PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

add_custom_command(TARGET GameVerseArena POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/dic.txt"
            "$<TARGET_FILE_DIR:GameVerseArena>/dic.txt"
    COMMENT "Copying Word Tic-Tac-Toe dictionary beside the executable"
)

add_executable(GameVerseArenaGUI
    "src/gui/main.cpp"
    "src/gui/Application.cpp"
    "src/gui/SceneManager.cpp"
    "src/gui/MainMenuScene.cpp"
    "src/gui/GameLibraryScene.cpp"
    "src/gui/SettingsScene.cpp"
    "src/gui/AboutScene.cpp"
    "src/gui/UiButton.cpp"
)

set_target_properties(GameVerseArenaGUI PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if(WIN32)
    set_target_properties(GameVerseArenaGUI PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(GameVerseArenaGUI PRIVATE "/ENTRY:mainCRTStartup")
    endif()
endif()

target_link_libraries(GameVerseArenaGUI PRIVATE SFML::Graphics)

add_custom_command(TARGET GameVerseArenaGUI POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GameVerseArenaGUI>/assets"
    COMMENT "Copying GUI assets beside the executable"
)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT GameVerseArenaGUI)
