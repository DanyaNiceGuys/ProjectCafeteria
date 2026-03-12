#Cafeteria       — основной исполняемый файл
#CafeteriaTests  — unit-тесты на Google Test

cmake_minimum_required(VERSION 3.20)
project(Cafeteria)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
set(ENV{PKG_CONFIG_PATH}
        "/opt/homebrew/opt/libpq/lib/pkgconfig:/opt/homebrew/lib/pkgconfig")

pkg_check_modules(PQXX REQUIRED libpqxx)

include_directories(${PQXX_INCLUDE_DIRS})
include_directories(/opt/homebrew/include)
include_directories(/opt/homebrew/opt/libpq/include)
include_directories(/opt/homebrew/opt/cpp-httplib/include)
include_directories(/opt/homebrew/opt/nlohmann-json/include)

link_directories(${PQXX_LIBRARY_DIRS})
link_directories(/opt/homebrew/opt/libpq/lib)

# Основной исполняемый файл
add_executable(Cafeteria
        main.cpp
        database.cpp
        tests.cpp
        console.h
)
target_link_libraries(Cafeteria ${PQXX_LIBRARIES} pq)

# Тесты (Google Test)
find_package(GTest QUIET)

if(GTest_FOUND)
    enable_testing()

    add_executable(CafeteriaTests
            tests.cpp
            database.cpp
    )

    target_link_libraries(CafeteriaTests
            GTest::GTest
            GTest::Main
            ${PQXX_LIBRARIES}
            pq
    )

    include(GoogleTest)
    gtest_discover_tests(CafeteriaTests)

    message(STATUS "Google Test найден — тесты собраны (таргет: CafeteriaTests)")
else()
    message(WARNING "Google Test не найден. Установи: brew install googletest")
endif()
