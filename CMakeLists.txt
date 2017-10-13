cmake_minimum_required(VERSION 2.6)
project(minotar)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Werror -Wextra -pedantic -std=c11")

add_library(minotar STATIC SHARED src/minotar_extract.c)
target_include_directories(minotar PUBLIC include)

add_subdirectory(examples/)

#enable_testing()
#add_subdirectory(test/)

install(TARGETS minotar LIBRARY DESTINATION lib/)
install(FILES "include/minotar.h" DESTINATION include/)
