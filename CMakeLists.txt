cmake_minimum_required(VERSION 3.28)

project(GameVerseArena LANGUAGES C CXX)

enable_testing()

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

# SFML 3.1.0 enables Freetype unity builds, which trigger third-party macro
# collisions with current MinGW/GNU 15. Keep the dependency pinned and build
# its original translation units separately.
if(TARGET freetype)
    set_target_properties(freetype PROPERTIES UNITY_BUILD OFF)
endif()

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
    "src/gui/TicTacToeSetupScene.cpp"
    "src/gui/TicTacToeGameScene.cpp"
    "src/gui/TicTacToeResultOverlay.cpp"
    "src/gui/PingPongSetupScene.cpp"
    "src/gui/PingPongGameScene.cpp"
    "src/gui/PingPongPauseOverlay.cpp"
    "src/gui/PingPongResultOverlay.cpp"
    "src/gui/ProfilesScene.cpp"
    "src/gui/ProfileStatsScene.cpp"
    "src/gui/ProfileAchievementsScene.cpp"
    "src/gui/MatchHistoryScene.cpp"
    "src/gui/AchievementToast.cpp"
    "src/gui/ProfileEditOverlay.cpp"
    "src/gui/ProfileDeleteOverlay.cpp"
)

add_library(GameVerseArenaSQLite STATIC
    "third_party/sqlite/sqlite3.c"
)

target_include_directories(GameVerseArenaSQLite PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite"
)

target_compile_definitions(GameVerseArenaSQLite PRIVATE
    SQLITE_OMIT_LOAD_EXTENSION
)

add_library(GameVerseArenaAchievements
    "src/achievements/AchievementCatalogue.cpp"
    "src/achievements/AchievementEvaluator.cpp"
    "src/achievements/AchievementNotificationQueue.cpp"
)

target_include_directories(GameVerseArenaAchievements PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/achievements"
)
target_compile_features(GameVerseArenaAchievements PUBLIC cxx_std_17)

add_library(GameVerseArenaPersistence
    "src/persistence/Database.cpp"
    "src/persistence/DatabasePaths.cpp"
    "src/persistence/MatchTypes.cpp"
    "src/persistence/MatchRepository.cpp"
    "src/persistence/MatchService.cpp"
    "src/persistence/StatisticsRepository.cpp"
    "src/persistence/AchievementRepository.cpp"
    "src/persistence/AchievementService.cpp"
    "src/persistence/ProfileRepository.cpp"
    "src/persistence/ProfileService.cpp"
)

target_include_directories(GameVerseArenaPersistence PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/persistence"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/achievements"
)

target_compile_features(GameVerseArenaPersistence PUBLIC cxx_std_17)
target_link_libraries(GameVerseArenaPersistence PUBLIC GameVerseArenaAchievements PRIVATE GameVerseArenaSQLite)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(GameVerseArenaAchievements PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(GameVerseArenaPersistence PRIVATE -Wall -Wextra -pedantic)
endif()

add_library(GameVerseArenaTicTacToe
    "src/games/classic_tic_tac_toe/TicTacToeBoard.cpp"
    "src/games/classic_tic_tac_toe/TicTacToeAI.cpp"
    "src/games/classic_tic_tac_toe/TicTacToeSession.cpp"
)

target_include_directories(GameVerseArenaTicTacToe PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/classic_tic_tac_toe"
)

target_compile_features(GameVerseArenaTicTacToe PUBLIC cxx_std_17)

add_executable(GameVerseArenaTests
    "tests/TicTacToeTests.cpp"
)

target_link_libraries(GameVerseArenaTests PRIVATE GameVerseArenaTicTacToe)
target_compile_features(GameVerseArenaTests PRIVATE cxx_std_17)
add_test(NAME ClassicTicTacToeTests COMMAND GameVerseArenaTests)

add_library(GameVerseArenaPingPong
    "src/games/ping_pong/PingPongSimulation.cpp"
    "src/games/ping_pong/PingPongAI.cpp"
    "src/games/ping_pong/PingPongSession.cpp"
)

target_include_directories(GameVerseArenaPingPong PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/ping_pong"
)

target_compile_features(GameVerseArenaPingPong PUBLIC cxx_std_17)

add_executable(GameVerseArenaPingPongTests
    "tests/PingPongTests.cpp"
)

target_link_libraries(GameVerseArenaPingPongTests PRIVATE GameVerseArenaPingPong)
target_compile_features(GameVerseArenaPingPongTests PRIVATE cxx_std_17)
add_test(NAME PingPongTests COMMAND GameVerseArenaPingPongTests)

add_executable(GameVerseArenaPersistenceTests
    "tests/PersistenceTests.cpp"
)

target_link_libraries(GameVerseArenaPersistenceTests PRIVATE GameVerseArenaPersistence)
target_compile_features(GameVerseArenaPersistenceTests PRIVATE cxx_std_17)
add_test(NAME PersistenceTests COMMAND GameVerseArenaPersistenceTests)

add_executable(GameVerseArenaMatchHistoryTests
    "tests/MatchHistoryTests.cpp"
)

target_link_libraries(GameVerseArenaMatchHistoryTests PRIVATE
    GameVerseArenaPersistence GameVerseArenaTicTacToe GameVerseArenaPingPong)
target_compile_features(GameVerseArenaMatchHistoryTests PRIVATE cxx_std_17)
add_test(NAME MatchHistoryTests COMMAND GameVerseArenaMatchHistoryTests)

add_executable(GameVerseArenaAchievementTests
    "tests/AchievementTests.cpp"
)

target_include_directories(GameVerseArenaAchievementTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/achievements"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
)
target_link_libraries(GameVerseArenaAchievementTests PRIVATE GameVerseArenaPersistence GameVerseArenaAchievements)
target_compile_features(GameVerseArenaAchievementTests PRIVATE cxx_std_17)
add_test(NAME AchievementTests COMMAND GameVerseArenaAchievementTests)

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

target_link_libraries(GameVerseArenaGUI PRIVATE
    GameVerseArenaTicTacToe
    GameVerseArenaPingPong
    GameVerseArenaPersistence
    SFML::Graphics
)

add_custom_command(TARGET GameVerseArenaGUI POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GameVerseArenaGUI>/assets"
    COMMENT "Copying GUI assets beside the executable"
)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT GameVerseArenaGUI)
