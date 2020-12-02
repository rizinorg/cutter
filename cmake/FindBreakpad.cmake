# - Find Breakpad
#
#  Breakpad_FOUND          - True if Breakpad has been found.
#  Breakpad_INCLUDE_DIRS   - Breakpad include directory
#  Breakpad_LIBRARIES      - List of libraries when using Breakpad.

set(Breakpad_LIBRARIES_VARS "")
if(WIN32)
	find_path(Breakpad_INCLUDE_DIRS
			client/windows/handler/exception_handler.h
			HINTS
				"${CMAKE_CURRENT_SOURCE_DIR}/Breakpad/src/src")

	set(Breakpad_LIBRARY_NAMES
			exception_handler
			crash_generation_client
			common
			)

	set(Breakpad_LIBRARIES "")
	
	foreach(libname ${Breakpad_LIBRARY_NAMES})
		find_library(Breakpad_LIBRARY_${libname}
				${libname}
				HINTS
					"${CMAKE_CURRENT_SOURCE_DIR}/Breakpad/src/src/client/windows/Release/lib"
				REQUIRED)

		list(APPEND Breakpad_LIBRARIES ${Breakpad_LIBRARY_${libname}})
		list(APPEND Breakpad_LIBRARIES_VARS "Breakpad_LIBRARY_${libname}")
	endforeach()
	
	set (Breakpad_LINK_LIBRARIES ${Breakpad_LIBRARIES})

	set(Breakpad_LIBRARY_DIRS "")
else()
	set(Breakpad_CMAKE_PREFIX_PATH_TEMP ${CMAKE_PREFIX_PATH})
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Breakpad/prefix")

	find_package(PkgConfig REQUIRED)
	pkg_search_module(Breakpad REQUIRED breakpad-client)

	# reset CMAKE_PREFIX_PATH
	set(CMAKE_PREFIX_PATH ${Breakpad_CMAKE_PREFIX_PATH_TEMP})
	mark_as_advanced(Breakpad_CMAKE_PREFIX_PATH_TEMP)
endif()

# could be simplified in > cmake 3.11 using pkg_search_module IMPORTED_TARGET [GLOBAL] but this would still be required for windows
add_library(Breakpad::client INTERFACE IMPORTED)
set_target_properties(Breakpad::client PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${Breakpad_INCLUDE_DIRS}")
set_target_properties(Breakpad::client PROPERTIES
	INTERFACE_LINK_LIBRARIES "${Breakpad_LINK_LIBRARIES}")


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Breakpad REQUIRED_VARS Breakpad_LIBRARIES Breakpad_INCLUDE_DIRS ${Breakpad_LIBRARIES_VARS})

mark_as_advanced(Breakpad_LIBRARIES_VARS)
 
