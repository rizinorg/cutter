# - Find Rizin (librz)
#
#  This module provides the following imported targets, if found:
#
#  Rizin::librz
#
#  This will define the following variables:
#  (but don't use them if you don't know what you are doing, use Rizin::librz)
#
#  Rizin_FOUND          - True if librz has been found.
#  Rizin_INCLUDE_DIRS   - librz include directory
#  Rizin_LIBRARIES      - List of libraries when using librz.
#  Rizin_LIBRARY_DIRS   - librz library directories
#
#  If librz was found using find_library and not pkg-config, the following variables will also be set:
#  Rizin_LIBRARY_<name> - Path to library rz_<name>

if(WIN32)
	find_path(Rizin_INCLUDE_DIRS
			NAMES rz_core.h rz_bin.h rz_util.h
			HINTS
				"$ENV{HOME}/bin/prefix/rizin/include/librz"
				/usr/local/include/libr
				/usr/include/librz)
        find_path(SDB_INCLUDE_DIR
			NAMES sdb.h sdbht.h sdb_version.h
			HINTS
				"$ENV{HOME}/bin/prefix/rizin/include/librz/sdb"
				/usr/local/include/librz/sdb
				/usr/include/librz/sdb)

        list(APPEND Rizin_INCLUDE_DIRS ${SDB_INCLUDE_DIR})

	set(Rizin_LIBRARY_NAMES
			core
			config
			cons
			io
			util
			flag
			asm
			debug
			hash
			bin
			lang
			io
			analysis
			parse
			bp
			egg
			reg
			search
			syscall
			socket
			magic
			crypto
			type)

	set(Rizin_LIBRARIES "")
	set(Rizin_LIBRARIES_VARS "")
	foreach(libname ${Rizin_LIBRARY_NAMES})
		find_library(Rizin_LIBRARY_${libname}
				rz_${libname}
				HINTS
					"$ENV{HOME}/bin/prefix/rizin/lib"
					/usr/local/lib
					/usr/lib)

		list(APPEND Rizin_LIBRARIES ${Rizin_LIBRARY_${libname}})
		list(APPEND Rizin_LIBRARIES_VARS "Rizin_LIBRARY_${libname}")
	endforeach()

	set(Rizin_LIBRARY_DIRS "")

	add_library(Rizin::librz UNKNOWN IMPORTED)
	set_target_properties(Rizin::librz PROPERTIES
			IMPORTED_LOCATION "${Rizin_LIBRARY_core}"
			IMPORTED_LINK_INTERFACE_LIBRARIES "${Rizin_LIBRARIES}"
			INTERFACE_LINK_DIRECTORIES "${Rizin_LIBRARY_DIRS}"
			INTERFACE_INCLUDE_DIRECTORIES "${Rizin_INCLUDE_DIRS}")
	set(Rizin_TARGET Rizin::librz)
else()
	# support installation locations used by rizin scripts like sys/user.sh and sys/install.sh
	if(CUTTER_USE_ADDITIONAL_RIZIN_PATHS)
		set(Rizin_CMAKE_PREFIX_PATH_TEMP ${CMAKE_PREFIX_PATH})
		list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/bin/prefix/rizin") # sys/user.sh
		list(APPEND CMAKE_PREFIX_PATH "/usr/local") # sys/install.sh
	endif()

	find_package(PkgConfig REQUIRED)
	if(CMAKE_VERSION VERSION_LESS "3.6")
		pkg_search_module(Rizin REQUIRED rz_core)
	else()
		pkg_search_module(Rizin IMPORTED_TARGET REQUIRED rz_core)
	endif()

	# reset CMAKE_PREFIX_PATH
	if(CUTTER_USE_ADDITIONAL_RIZIN_PATHS)
		set(CMAKE_PREFIX_PATH ${Rizin_CMAKE_PREFIX_PATH_TEMP})
	endif()

	if((TARGET PkgConfig::Rizin) AND (NOT CMAKE_VERSION VERSION_LESS "3.11.0"))
		set_target_properties(PkgConfig::Rizin PROPERTIES IMPORTED_GLOBAL ON)
		add_library(Rizin::librz ALIAS PkgConfig::Rizin)
		set(Rizin_TARGET Rizin::librz)
	elseif(Rizin_FOUND)
		add_library(Rizin::librz INTERFACE IMPORTED)
		set_target_properties(Rizin::librz PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES "${Rizin_INCLUDE_DIRS}")
		set_target_properties(Rizin::librz PROPERTIES
			INTERFACE_LINK_LIBRARIES "${Rizin_LIBRARIES}")
		set(Rizin_TARGET Rizin::librz)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Rizin REQUIRED_VARS Rizin_TARGET Rizin_LIBRARIES Rizin_INCLUDE_DIRS)
