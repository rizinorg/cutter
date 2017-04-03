# - Find Radare2 (libr)
#
#  RADARE2_FOUND          - True if libr has been found.
#  RADARE2_INCLUDE_DIRS   - libr Include Directory
#  RADARE2_LIBRARIES      - List of libraries when using libr.
#  RADARE2_LIBRARY_<name> - Path to library r_<name>

find_path(RADARE2_INCLUDE_DIRS
		NAMES r_core.h r_bin.h r_util.h
		HINTS
			"$ENV{HOME}/bin/prefix/radare2/include/libr"
			/usr/local/include/libr
			/usr/include/libr)

set(RADARE2_LIBRARY_NAMES
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
		anal
		parse
		bp
		egg
		reg
		search
		syscall
		socket
		fs
		magic
		crypto)

set(RADARE2_LIBRARIES "")
set(RADARE2_LIBRARIES_VARS "")
foreach(libname ${RADARE2_LIBRARY_NAMES})
	find_library(RADARE2_LIBRARY_${libname}
			r_${libname}
			HINTS
				"$ENV{HOME}/bin/prefix/radare2/lib"
				/usr/local/lib
				/usr/lib)

	list(APPEND RADARE2_LIBRARIES ${RADARE2_LIBRARY_${libname}})
	list(APPEND RADARE2_LIBRARIES_VARS "RADARE2_LIBRARY_${libname}")
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RADARE2 REQUIRED_VARS ${RADARE2_LIBRARIES_VARS} RADARE2_INCLUDE_DIRS)

mark_as_advanced(RADARE2_LIBRARIES_VARS)
