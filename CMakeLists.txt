cmake_minimum_required(VERSION 3.16)
project(WCtestbeamSim)

# Forza CMake a cercare prima in Conda per TUTTI i pacchetti (ZLIB inclusa)
if(DEFINED ENV{CONDA_PREFIX})
    set(CMAKE_PREFIX_PATH $ENV{CONDA_PREFIX} ${CMAKE_PREFIX_PATH})
    set(CMAKE_FIND_FRAMEWORK LAST)
    set(CMAKE_FIND_APPBUNDLE LAST)
    message(STATUS "Priorità librerie impostata su Conda: $ENV{CONDA_PREFIX}")
endif()

# Trova Geant4 nell'ambiente Conda
find_package(Geant4 REQUIRED ui_all vis_all)

# Include le macro di Geant4
include(${Geant4_USE_FILE})

# Elenca i file sorgente
include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# Crea l'eseguibile
add_executable(WCtestbeamSim sim.cc ${sources} ${headers})
target_link_libraries(WCtestbeamSim ${Geant4_LIBRARIES})
