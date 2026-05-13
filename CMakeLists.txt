cmake_minimum_required(VERSION 3.14)
project(s21_bot)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

include(FetchContent)

FetchContent_Declare(
    tgbot
    GIT_REPOSITORY https://github.com/reo7sp/tgbot-cpp.git
    GIT_TAG master
)

FetchContent_MakeAvailable(tgbot)

add_executable(s21_bot
    src/main.cpp
    src/api/S21Api.cpp
    src/bot/PeerFinderBot.cpp
)

target_link_libraries(s21_bot 
    OpenSSL::SSL 
    OpenSSL::Crypto
    TgBot
)

target_include_directories(s21_bot PRIVATE 
    /usr/include
)

target_compile_definitions(s21_bot PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)