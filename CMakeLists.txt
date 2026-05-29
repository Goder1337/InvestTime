cmake_minimum_required(VERSION 3.16)

project(StudyWithMe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_PACKAGE Qt5)
endif()

add_executable(StudyWithMe
    main.cpp
)

target_link_libraries(StudyWithMe PRIVATE ${QT_PACKAGE}::Widgets)
