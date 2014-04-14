cmake_minimum_required (VERSION 2.8)
project(test)

add_definitions("-Wall")
add_definitions("-g")
add_definitions("-fexceptions")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")	
#defines
#add_definitions(-DBOOST_ASIO_DYN_LINK)


#tests
add_executable(test main.cpp sqlite3/sqlite3.c)
target_link_libraries(test c++ boost_serialization)
	    

