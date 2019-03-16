# - Find Breakpad
#
#  BREAKPAD_FOUND          - True if Breakpad has been found.
#  BREAKPAD_INCLUDE_DIRS   - Breakpad include directory
#  BREAKPAD_LIBRARIES      - List of libraries when using Breakpad.
#  BREAKPAD_LIBRARY_DIRS   - Breakpad library directories

if(WIN32)
	find_path(BREAKPAD_INCLUDE_DIRS
			HINTS
				"${CMAKE_CURRENT_SOURCE_DIR}/breakpad/prefix/include/breakpad")

	set(BREAKPAD_LIBRARY_NAMES
			BREAKPAD_CLIENT
			BREAKPAD)

	set(BREAKPAD_LIBRARIES "")
	set(BREAKPAD_LIBRARIES_VARS "")
	foreach(libname ${BREAKPAD_LIBRARY_NAMES})
		find_library(BREAKPAD_LIBRARY_${libname}
				${libname}
				HINTS
					"${CMAKE_CURRENT_SOURCE_DIR}/breakpad/prefix/lib")

		list(APPEND BREAKPAD_LIBRARIES ${BREAKPAD_LIBRARY_${libname}})
		list(APPEND BREAKPAD_LIBRARIES_VARS "BREAKPAD_LIBRARY_${libname}")
	endforeach()

	set(BREAKPAD_LIBRARY_DIRS "")
else()
	set(BREAKPAD_CMAKE_PREFIX_PATH_TEMP ${CMAKE_PREFIX_PATH})
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/breakpad/prefix")

	find_package(PkgConfig REQUIRED)
	pkg_search_module(BREAKPAD REQUIRED breakpad-client)

	# reset CMAKE_PREFIX_PATH
	set(CMAKE_PREFIX_PATH ${BREAKPAD_CMAKE_PREFIX_PATH_TEMP})
	mark_as_advanced(BREAKPAD_CMAKE_PREFIX_PATH_TEMP)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BREAKPAD REQUIRED_VARS BREAKPAD_LIBRARIES BREAKPAD_INCLUDE_DIRS)

mark_as_advanced(BREAKPAD_LIBRARIES_VARS)
 
