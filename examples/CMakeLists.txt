cmake_minimum_required(VERSION 2.6)
project(minotar_examples)

add_executable(basic_minotar_stream basic_tar_stream.c)
target_link_libraries(basic_minotar_stream PUBLIC minotar)

add_executable(minotar_gzip tar_gz_extract.c)
target_link_libraries(minotar_gzip PUBLIC minotar z)
