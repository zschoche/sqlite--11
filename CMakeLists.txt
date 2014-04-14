cmake_minimum_required (VERSION 2.8)
project(test)

#defines
add_definitions("-Wall")
add_definitions("-g")
add_definitions("-fexceptions")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")	

#libraries
add_library(sqlitepp11 sqlite3/sqlite3.c)


#tests
add_executable(test main.cpp)
target_link_libraries(test c++ sqlitepp11)
	    

