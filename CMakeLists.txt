cmake_minimum_required(VERSION 3.16)
project(AppMed VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration Qt automatique
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

# On indique à CMake où trouver les headers (.h)
# Cela permet de faire #include "core/simulation.h"
include_directories(include)

# Définition des sources
set(SOURCES
    app/main.cpp
    src/core/simulation.cpp
    src/core/patient.cpp     # Si vous avez séparé la classe Patient
    src/ui/gui.cpp
    src/ui/home.cpp
    src/ui/realtime.cpp
    resources/resources.qrc  # On compile les ressources (CSS) dans l'exécutable

    include/ui/home.h
    include/ui/gui.h
    include/ui/realtime.h
)

# Création de l'exécutable
add_executable(AppMed ${SOURCES})

target_link_libraries(AppMed PRIVATE Qt5::Widgets)

# Tests (optionnel, si vous voulez compiler les tests)
enable_testing()
add_subdirectory(tests)