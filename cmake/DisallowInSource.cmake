function(DisallowInSource)
	get_filename_component(SRC_DIR_REALPATH "${CMAKE_SOURCE_DIR}" REALPATH)
	get_filename_component(BINARY_DIR_REALPATH "${CMAKE_BINARY_DIR}" REALPATH)
	if(SRC_DIR_REALPATH STREQUAL BINARY_DIR_REALPATH)
		message(FATAL_ERROR " In-source builds are not allowed.
 Please create a directory and run cmake from there:
 mkdir build && cd build && cmake ..
 This process created the file CMakeCache.txt and the directory CMakeFiles in ${CMAKE_SOURCE_DIR}.
 Please delete them manually!")
	endif()
endfunction()

DisallowInSource()