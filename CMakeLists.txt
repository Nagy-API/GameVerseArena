cmake_minimum_required(VERSION 3.20)

project(GameVerseArena LANGUAGES CXX)

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
