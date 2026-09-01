cmake_minimum_required(VERSION 3.10)
project(mud3)

set(CMAKE_CXX_STANDARD 20)

add_executable(mud3
    main.cpp
    Item.cpp
    Player.cpp
    Game.cpp
)
