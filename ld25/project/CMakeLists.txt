cmake_minimum_required(VERSION 2.8)

project (ld25)

set (SRC
	"Contracts.cpp"
	"Server.cpp"
	"Session.cpp"
	"Users.cpp"
	"Util.cpp"
	"World.cpp"
	"main.cpp"
	"LawlJSON/LawlJSON.cpp"
	"LawlJSON/LawlJSON_Lexer.cpp"
	"LawlJSON/LawlJSON_Tokens.cpp"
	"LawlJSON/LawlJSON_Types.cpp"
)

SET (INC
	"Colors.h"
	"Contracts.h"
	"Log.h"
	"Server.h"
	"Session.h"
	"Settings.h"
	"Users.h"
	"Util.h"
	"World.h"
	"LawlJSON/LawlJSON.h"
	"LawlJSON/LawlJSON_Exception.h"
	"LawlJSON/LawlJSON_Lexer.h"
	"LawlJSON/LawlJSON_Settings.h"
	"LawlJSON/LawlJSON_Tokens.h"
	"LawlJSON/LawlJSON_Types.h"
)

#set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost)

include_directories(${Boost_INCLUDE_DIRS} "LawlJSON/")

add_definitions(-O2)

add_executable(${PROJECT_NAME} ${SRC} ${INC})
target_link_libraries(${PROJECT_NAME} pthread boost_system boost_date_time)
