project (optee_example_my_test C)

set (SRC host/main.c
	 host/sqlite3.c
	 host/defs.c
	 host/dbqueue.c
	 host/mhtdefs.c
	 host/mhtfile.c
	 host/sqlite3.h
	 host/defs.h
	 host/dbqueue.h
	 host/mhtdefs.h
	 host/mhtfile.h)

add_executable (${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include
			   PRIVATE include)

target_link_libraries (${PROJECT_NAME} PRIVATE teec pthread m crypto dl)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
