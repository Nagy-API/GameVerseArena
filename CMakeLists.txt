cmake_minimum_required(VERSION 3.28)

# The project version below is the single source of the application version. It is exposed
# to the GUI through the generated Version.hpp header.
project(GameVerseArena VERSION 0.9.0 LANGUAGES C CXX)

enable_testing()

include(FetchContent)

set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
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

# Strict diagnostics for project-owned code. Third-party code (SFML and its
# dependencies, the SQLite amalgamation) and the legacy console sources are excluded.
function(gva_strict_warnings target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
    elseif(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Console application (the original 14 board games; no SFML or SQLite)
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# SFML-independent libraries
# ---------------------------------------------------------------------------
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
gva_strict_warnings(GameVerseArenaAchievements)

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
    "src/persistence/SettingsRepository.cpp"
    "src/persistence/SettingsService.cpp"
)

target_include_directories(GameVerseArenaPersistence PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/persistence"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/achievements"
)

target_compile_features(GameVerseArenaPersistence PUBLIC cxx_std_17)
target_link_libraries(GameVerseArenaPersistence PUBLIC GameVerseArenaAchievements PRIVATE GameVerseArenaSQLite)
gva_strict_warnings(GameVerseArenaPersistence)

add_library(GameVerseArenaAudio
    "src/audio/SoundTypes.cpp"
    "src/audio/ProceduralSynth.cpp"
    "src/audio/AudioPolicy.cpp"
)
target_include_directories(GameVerseArenaAudio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/audio")
target_compile_features(GameVerseArenaAudio PUBLIC cxx_std_17)
gva_strict_warnings(GameVerseArenaAudio)

add_library(GameVerseArenaTicTacToe
    "src/games/classic_tic_tac_toe/TicTacToeBoard.cpp"
    "src/games/classic_tic_tac_toe/TicTacToeAI.cpp"
    "src/games/classic_tic_tac_toe/TicTacToeSession.cpp"
)

target_include_directories(GameVerseArenaTicTacToe PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/classic_tic_tac_toe"
)

target_compile_features(GameVerseArenaTicTacToe PUBLIC cxx_std_17)
gva_strict_warnings(GameVerseArenaTicTacToe)

add_library(GameVerseArenaPingPong
    "src/games/ping_pong/PingPongSimulation.cpp"
    "src/games/ping_pong/PingPongAI.cpp"
    "src/games/ping_pong/PingPongSession.cpp"
)

target_include_directories(GameVerseArenaPingPong PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/ping_pong"
)

target_compile_features(GameVerseArenaPingPong PUBLIC cxx_std_17)
gva_strict_warnings(GameVerseArenaPingPong)

# The original board games rebuilt for the graphical app: SFML-independent rules and computer
# strategies behind the shared turn-based contract, plus the session and background search.
find_package(Threads REQUIRED)
add_library(GameVerseArenaBoardGames
    "src/games/common/BoardGameSession.cpp"
    "src/games/common/ComputerMoveTask.cpp"
    "src/games/numerical/NumericalGame.cpp"
    "src/games/sus/SusGame.cpp"
    "src/games/five_by_five/FiveByFiveGame.cpp"
    "src/games/misere/MisereGame.cpp"
    "src/games/four_in_a_row/FourInRowGame.cpp"
    "src/games/four_by_four/FourByFourGame.cpp"
    "src/games/pyramid/PyramidGame.cpp"
    "src/games/diamond/DiamondGame.cpp"
    "src/games/word/WordGame.cpp"
    "src/games/infinity/InfinityGame.cpp"
    "src/games/memory/MemoryGame.cpp"
    "src/games/obstacle/ObstacleGame.cpp"
)
target_include_directories(GameVerseArenaBoardGames PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/numerical"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/sus"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/five_by_five"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/misere"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/four_in_a_row"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/four_by_four"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/pyramid"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/diamond"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/word"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/infinity"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/memory"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/games/obstacle"
)
target_compile_features(GameVerseArenaBoardGames PUBLIC cxx_std_17)
target_link_libraries(GameVerseArenaBoardGames PUBLIC Threads::Threads)
gva_strict_warnings(GameVerseArenaBoardGames)

# The graphical game catalogue (registry metadata and board-game factories).
add_library(GameVerseArenaCatalogue
    "src/catalogue/GameCatalogue.cpp"
)
target_include_directories(GameVerseArenaCatalogue PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/catalogue"
)
target_compile_features(GameVerseArenaCatalogue PUBLIC cxx_std_17)
target_link_libraries(GameVerseArenaCatalogue PUBLIC GameVerseArenaBoardGames)
gva_strict_warnings(GameVerseArenaCatalogue)

# ---------------------------------------------------------------------------
# Tests (no window, no audio device, temporary databases only)
# ---------------------------------------------------------------------------
add_executable(GameVerseArenaCatalogueTests
    "tests/GameCatalogueTests.cpp"
)
target_link_libraries(GameVerseArenaCatalogueTests PRIVATE GameVerseArenaCatalogue GameVerseArenaPersistence)
target_compile_definitions(GameVerseArenaCatalogueTests PRIVATE
    GVA_CONSOLE_MENU_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/XO_Demo.cpp")
target_compile_features(GameVerseArenaCatalogueTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaCatalogueTests)
add_test(NAME GameCatalogueTests COMMAND GameVerseArenaCatalogueTests)

# Rules, computer strategy, and contract tests for each graphical board game.
function(gva_board_game_test name)
    add_executable(GameVerseArena${name} "tests/${name}.cpp")
    target_link_libraries(GameVerseArena${name} PRIVATE GameVerseArenaBoardGames)
    target_compile_features(GameVerseArena${name} PRIVATE cxx_std_17)
    gva_strict_warnings(GameVerseArena${name})
    add_test(NAME ${name} COMMAND GameVerseArena${name})
endfunction()

gva_board_game_test(BoardGameSessionTests)
gva_board_game_test(NumericalGameTests)
gva_board_game_test(SusGameTests)
gva_board_game_test(FiveByFiveGameTests)
gva_board_game_test(MisereGameTests)
gva_board_game_test(FourInRowGameTests)
gva_board_game_test(FourByFourGameTests)
gva_board_game_test(PyramidGameTests)
gva_board_game_test(DiamondGameTests)
gva_board_game_test(WordGameTests)
# The built-in Word dictionary must stay identical to the console's dic.txt.
target_compile_definitions(GameVerseArenaWordGameTests PRIVATE
    GVA_WORD_DICTIONARY_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/dic.txt")
gva_board_game_test(InfinityGameTests)
gva_board_game_test(MemoryGameTests)
gva_board_game_test(ObstacleGameTests)

add_executable(GameVerseArenaTests
    "tests/TicTacToeTests.cpp"
)

target_link_libraries(GameVerseArenaTests PRIVATE GameVerseArenaTicTacToe)
target_compile_features(GameVerseArenaTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaTests)
add_test(NAME ClassicTicTacToeTests COMMAND GameVerseArenaTests)

add_executable(GameVerseArenaPingPongTests
    "tests/PingPongTests.cpp"
)

target_link_libraries(GameVerseArenaPingPongTests PRIVATE GameVerseArenaPingPong)
target_compile_features(GameVerseArenaPingPongTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaPingPongTests)
add_test(NAME PingPongTests COMMAND GameVerseArenaPingPongTests)

add_executable(GameVerseArenaPersistenceTests
    "tests/PersistenceTests.cpp"
)

target_link_libraries(GameVerseArenaPersistenceTests PRIVATE GameVerseArenaPersistence)
target_compile_features(GameVerseArenaPersistenceTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaPersistenceTests)
add_test(NAME PersistenceTests COMMAND GameVerseArenaPersistenceTests)

add_executable(GameVerseArenaMatchHistoryTests
    "tests/MatchHistoryTests.cpp"
)

target_link_libraries(GameVerseArenaMatchHistoryTests PRIVATE
    GameVerseArenaPersistence GameVerseArenaTicTacToe GameVerseArenaPingPong)
target_compile_features(GameVerseArenaMatchHistoryTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaMatchHistoryTests)
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
gva_strict_warnings(GameVerseArenaAchievementTests)
add_test(NAME AchievementTests COMMAND GameVerseArenaAchievementTests)

add_executable(GameVerseArenaSettingsTests
    "tests/SettingsTests.cpp"
)
# SQLite is linked directly only so a test can inspect a database the migrating wrapper refuses to open.
target_link_libraries(GameVerseArenaSettingsTests PRIVATE GameVerseArenaPersistence GameVerseArenaSQLite)
target_compile_features(GameVerseArenaSettingsTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaSettingsTests)
add_test(NAME SettingsTests COMMAND GameVerseArenaSettingsTests)

add_executable(GameVerseArenaAudioTests
    "tests/AudioTests.cpp"
)
target_link_libraries(GameVerseArenaAudioTests PRIVATE GameVerseArenaAudio)
target_compile_features(GameVerseArenaAudioTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaAudioTests)
add_test(NAME AudioTests COMMAND GameVerseArenaAudioTests)

add_executable(GameVerseArenaSchemaSafetyTests
    "tests/SchemaSafetyTests.cpp"
)
# SQLite is linked directly only so tests can craft and inspect files the wrapper refuses.
target_link_libraries(GameVerseArenaSchemaSafetyTests PRIVATE GameVerseArenaPersistence GameVerseArenaSQLite)
target_compile_features(GameVerseArenaSchemaSafetyTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaSchemaSafetyTests)
add_test(NAME SchemaSafetyTests COMMAND GameVerseArenaSchemaSafetyTests)

# The SFML-backed audio engine and settings controller, run on SFML's silent null device so
# no sound hardware is required.
add_executable(GameVerseArenaAudioEngineTests
    "tests/AudioEngineTests.cpp"
    "src/gui/AudioEngine.cpp"
    "src/gui/SettingsController.cpp"
)
target_include_directories(GameVerseArenaAudioEngineTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/gui")
target_link_libraries(GameVerseArenaAudioEngineTests PRIVATE
    GameVerseArenaAudio GameVerseArenaPersistence SFML::Audio)
target_compile_features(GameVerseArenaAudioEngineTests PRIVATE cxx_std_17)
gva_strict_warnings(GameVerseArenaAudioEngineTests)
add_test(NAME AudioEngineTests COMMAND GameVerseArenaAudioEngineTests)

# ---------------------------------------------------------------------------
# Graphical application
# ---------------------------------------------------------------------------
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Version.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.hpp" @ONLY)

add_executable(GameVerseArenaGUI
    "src/gui/main.cpp"
    "src/gui/Application.cpp"
    "src/gui/SceneManager.cpp"
    "src/gui/AudioEngine.cpp"
    "src/gui/SettingsController.cpp"
    "src/gui/ErrorDialog.cpp"
    "src/gui/SmokeTestDriver.cpp"
    "src/gui/MainMenuScene.cpp"
    "src/gui/GameLibraryScene.cpp"
    "src/gui/GameLauncher.cpp"
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
    "src/gui/BoardGameSetupScene.cpp"
    "src/gui/BoardGameScene.cpp"
    "src/gui/BoardGameResultOverlay.cpp"
    "src/gui/BoardGameRulesOverlay.cpp"
    "src/gui/board/BoardViews.cpp"
    "src/gui/board/CellBoardView.cpp"
    "src/gui/board/NumericalBoardView.cpp"
    "src/gui/board/SusBoardView.cpp"
    "src/gui/board/FiveByFiveBoardView.cpp"
    "src/gui/board/MisereBoardView.cpp"
    "src/gui/board/FourInRowBoardView.cpp"
    "src/gui/board/FourByFourBoardView.cpp"
    "src/gui/board/PyramidBoardView.cpp"
    "src/gui/board/DiamondBoardView.cpp"
    "src/gui/board/WordBoardView.cpp"
    "src/gui/board/InfinityBoardView.cpp"
    "src/gui/board/MemoryBoardView.cpp"
    "src/gui/board/ObstacleBoardView.cpp"
    "src/gui/ProfilesScene.cpp"
    "src/gui/ProfileStatsScene.cpp"
    "src/gui/ProfileAchievementsScene.cpp"
    "src/gui/MatchHistoryScene.cpp"
    "src/gui/AchievementToast.cpp"
    "src/gui/ProfileEditOverlay.cpp"
    "src/gui/ProfileDeleteOverlay.cpp"
)

set_target_properties(GameVerseArenaGUI PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
target_include_directories(GameVerseArenaGUI PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/board"
)
gva_strict_warnings(GameVerseArenaGUI)

if(WIN32)
    set_target_properties(GameVerseArenaGUI PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(GameVerseArenaGUI PRIVATE "/ENTRY:mainCRTStartup")
    endif()
    # CommandLineToArgvW: read UTF-16 command-line arguments so non-ASCII paths work.
    target_link_libraries(GameVerseArenaGUI PRIVATE shell32)
endif()

target_link_libraries(GameVerseArenaGUI PRIVATE
    GameVerseArenaTicTacToe
    GameVerseArenaPingPong
    GameVerseArenaPersistence
    GameVerseArenaAudio
    GameVerseArenaCatalogue
    SFML::Graphics
    SFML::Audio
)

add_custom_command(TARGET GameVerseArenaGUI POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:GameVerseArenaGUI>/assets"
    COMMENT "Copying GUI assets beside the executable"
)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT GameVerseArenaGUI)
