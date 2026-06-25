cmake_minimum_required(VERSION 3.16)
project(NextGenBanking VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Advertencias estrictas: tratar el codigo con seriedad.
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Biblioteca con la logica de negocio, reutilizable y testeable de forma aislada.
add_library(banking_core
    src/Account.cpp
    src/Bank.cpp
    src/Security.cpp
)
target_include_directories(banking_core PUBLIC include)

# Ejecutable principal: la interfaz de linea de comandos.
add_executable(banking src/main.cpp)
target_link_libraries(banking PRIVATE banking_core)

# Tests unitarios.
enable_testing()
add_executable(banking_tests tests/tests.cpp)
target_link_libraries(banking_tests PRIVATE banking_core)
add_test(NAME unit_tests COMMAND banking_tests)
