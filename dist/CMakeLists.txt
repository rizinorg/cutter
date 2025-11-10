set(CUTTER_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../cutter-deps")
unset(RZ_GHIDRA_PREFIX_PATH)

################################################
# Windows
################################################

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
	set(RIZIN_INSTALL_PLUGDIR "lib/rizin/plugins")

	if (CUTTER_PACKAGE_DEPENDENCIES)
		if (CUTTER_ENABLE_PYTHON)
			if (CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
				set(CPACK_INSTALL_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/WindowsBundlePython.cmake)
			endif()
			find_package (Python3 ${CUTTER_PYTHON_MIN} REQUIRED COMPONENTS)
			install(DIRECTORY ${CUTTER_DEPS}/pyside/lib/site-packages DESTINATION "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
			install(FILES ${CUTTER_DEPS}/pyside/bin/shiboken6.abi3.dll ${CUTTER_DEPS}/pyside/bin/pyside6.abi3.dll DESTINATION .)
		endif()
		install(SCRIPT WindowsBundleQt.cmake)
	endif()
	if (CUTTER_PACKAGE_JSDEC AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			set(ENV{RZ_PREFIX} \"\${CMAKE_INSTALL_PREFIX}\")
			set(ENV{PATH} \"\${CMAKE_INSTALL_PREFIX};\$ENV{PATH}\")
			execute_process(COMMAND powershell \"${CMAKE_CURRENT_SOURCE_DIR}/bundle_jsdec.ps1\"
							\"\${CMAKE_INSTALL_PREFIX}\"
							\"-DCUTTER_INSTALL_PLUGDIR=plugins/native\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package jsdec (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_LIBSWIFT AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			set(ENV{RZ_PREFIX} \"\${CMAKE_INSTALL_PREFIX}\")
			set(ENV{PATH} \"\${CMAKE_INSTALL_PREFIX};\$ENV{PATH}\")
			execute_process(COMMAND powershell \"${CMAKE_CURRENT_SOURCE_DIR}/bundle_rz_libswift.ps1\" \"\${CMAKE_INSTALL_PREFIX}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz-libswift (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_LIBYARA AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			set(ENV{RZ_PREFIX} \"\${CMAKE_INSTALL_PREFIX}\")
			set(ENV{PATH} \"\${CMAKE_INSTALL_PREFIX};\$ENV{PATH}\")
			execute_process(COMMAND powershell 
							\"${CMAKE_CURRENT_SOURCE_DIR}/bundle_rz_libyara.ps1\"
							\"\${CMAKE_INSTALL_PREFIX}\"
							\"-DCUTTER_INSTALL_PLUGDIR=plugins/native\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz_libyara (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_SILHOUETTE AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			set(ENV{RZ_PREFIX} \"\${CMAKE_INSTALL_PREFIX}\")
			set(ENV{PATH} \"\${CMAKE_INSTALL_PREFIX};\$ENV{PATH}\")
			execute_process(COMMAND powershell \"${CMAKE_CURRENT_SOURCE_DIR}/bundle_rz_silhouette.ps1\" \"\${CMAKE_INSTALL_PREFIX}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz-silhouette (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
endif()

################################################
# macOS
################################################

if(APPLE)
	set(CPACK_GENERATOR "Bundle")
	set(CPACK_BUNDLE_NAME "Cutter")
	set(ICON "Cutter")
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/macos/Info.plist")
	set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_BINARY_DIR}/macos/Info.plist")
	set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/macos/cutter.icns")
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/macos/cutter_mac_app.png")
	set(CPACK_DMG_DS_STORE "${CMAKE_CURRENT_SOURCE_DIR}/macos/DS_Store_ForDMGBackground")
	set(CPACK_DMG_VOLUME_NAME "Cutter")
	set(CPACK_BUNDLE_APPLE_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/macos/Entitlements.plist")
	set(CPACK_APPLE_BUNDLE_ID "re.rizin.cutter")

	find_program(MACDEPLOYQT_PATH macdeployqt HINTS "${Qt5_DIR}/../../../bin")
	if(NOT MACDEPLOYQT_PATH)
		message(FATAL_ERROR "Failed to find macdeployqt")
	endif()

	unset(ADJUST_RIZIN_LIBS)
	foreach(_lib ${RZ_LIBS})
		list(APPEND ADJUST_RIZIN_LIBS "${RIZIN_INSTALL_DIR}/lib/lib${_lib}.dylib")
	endforeach()
	foreach(_lib ${RZ_EXTRA_LIBS})
		list(APPEND ADJUST_RIZIN_LIBS "${RIZIN_INSTALL_DIR}/lib/lib${_lib}.dylib")
	endforeach()

	if(CUTTER_PACKAGE_DEPENDENCIES AND CUTTER_ENABLE_PYTHON)
		set(EMBED_PYTHON_SH "${CMAKE_CURRENT_SOURCE_DIR}/appbundle_embed_python.sh")
		set(PYTHON_FRAMEWORK_DIR "${CUTTER_DEPS}/python/Library/Frameworks/Python.framework")
		set(PYSIDE_PREFIX "${CUTTER_DEPS}/pyside")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/MacOSBundlePython.cmake.in"
			"${CMAKE_CURRENT_BINARY_DIR}/MacOSBundlePython.cmake" @ONLY)
		install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/MacOSBundlePython.cmake")
	endif()

	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/MacOSSetupBundle.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/MacOSSetupBundle.cmake" @ONLY)
	install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/MacOSSetupBundle.cmake")

	get_filename_component(QT_PREFIX "${MACDEPLOYQT_PATH}/../.." ABSOLUTE)
	list(APPEND RZ_GHIDRA_PREFIX_PATH "${QT_PREFIX}")
	set(RIZIN_INSTALL_PLUGDIR "lib/rizin/plugins") # escaped backslash on purpose, should be evaluated later
endif()

################################################
# macOS + Linux
################################################

if(CUTTER_ENABLE_DEPENDENCY_DOWNLOADS AND (NOT WIN32))
	if (CUTTER_PACKAGE_JSDEC)
		if(APPLE)
			set (JSDEC_PLUGIN_OPTIONS "-DCUTTER_INSTALL_PLUGDIR=plugins/native")
		else()
			set (JSDEC_PLUGIN_OPTIONS "")
		endif()
		install(CODE "
			execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/../scripts/jsdec.sh\"
							\"\${CMAKE_INSTALL_PREFIX}\" \"${JSDEC_PLUGIN_OPTIONS}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package jsdec (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_LIBSWIFT)
		install(CODE "
			execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/../scripts/rz-libswift.sh\"
							--pkg-config-path=\${CMAKE_INSTALL_PREFIX}/lib/pkgconfig --prefix=\${CMAKE_INSTALL_PREFIX}
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz-libswift (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_LIBYARA)
		if(APPLE)
			set (YARA_PLUGIN_OPTIONS "-DCUTTER_INSTALL_PLUGDIR=plugins/native")
		else()
			set (YARA_PLUGIN_OPTIONS "")
		endif()
		if (CUTTER_QT EQUAL 6)
			set (YARA_PLUGIN_OPTIONS "${YARA_PLUGIN_OPTIONS} -DCUTTER_QT6=ON")
		endif()
		install(CODE "
			execute_process(COMMAND
							\"${CMAKE_CURRENT_SOURCE_DIR}/../scripts/rz-libyara.sh\"
							\"\${CMAKE_INSTALL_PREFIX}\" \"${YARA_PLUGIN_OPTIONS}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz-libyara (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
	if (CUTTER_PACKAGE_RZ_SILHOUETTE)
		install(CODE "
			execute_process(COMMAND
							\"${CMAKE_CURRENT_SOURCE_DIR}/../scripts/rz-silhouette.sh\"
							\"\${CMAKE_INSTALL_PREFIX}\" \"${YARA_PLUGIN_OPTIONS}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package rz-silhouette (returned \${SCRIPT_RESULT})\")
			endif()
		")
	endif()
endif()

################################################
# rz-ghidra
################################################

if(CUTTER_PACKAGE_RZ_GHIDRA)
	if(CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		# Currently using external project only for downloading source
		# It neeeds to link against compiled cutter so for now build it using custom install script.
		# That way rz-ghidra build process is the same as with any other external plugin built against
		# installed Cutter.
		ExternalProject_Add(rz-ghidra
			GIT_REPOSITORY https://github.com/rizinorg/rz-ghidra
			#GIT_TAG v0.3.0
			#GIT_TAG c7a50a2e7c0a95cd52b167c9ee0fa1805223f08e
			GIT_TAG dev
			#GIT_SHALLOW ON # disable this line when using commit hash
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ""
			INSTALL_COMMAND ""
		)
	endif()
	if (WIN32 OR APPLE)
		set (GHIDRA_OPTIONS "
			\"-DRIZIN_INSTALL_PLUGDIR=\${CMAKE_INSTALL_PREFIX}/${RIZIN_INSTALL_PLUGDIR}\"
			-DCUTTER_INSTALL_PLUGDIR=plugins/native")
	else()
		set (GHIDRA_OPTIONS "-DCUTTER_INSTALL_PLUGDIR=\${CMAKE_INSTALL_PREFIX}/share/rizin/cutter/plugins/native")
	endif()
	install(CODE "
		execute_process(
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rz-ghidra-prefix/src/rz-ghidra-build
			RESULT_VARIABLE SCRIPT_RESULT
			COMMAND \${CMAKE_COMMAND}
							../rz-ghidra
							-DCMAKE_BUILD_TYPE=Release
							\"-DCMAKE_PREFIX_PATH=\${CMAKE_INSTALL_PREFIX};\${CMAKE_INSTALL_PREFIX}/include/librz;\${CMAKE_INSTALL_PREFIX}/include/librz/sdb;${RZ_GHIDRA_PREFIX_PATH}\"
							-DCMAKE_INSTALL_PREFIX=\${CMAKE_INSTALL_PREFIX}
							${GHIDRA_OPTIONS}
							-DBUILD_CUTTER_PLUGIN=ON
							-DBUILD_SLEIGH_PLUGIN=OFF
							-G Ninja
			)
		if (SCRIPT_RESULT)
			message(FATAL_ERROR \"Failed to configure rz-ghidra (returned \${SCRIPT_RESULT})\")
		endif()
		execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rz-ghidra-prefix/src/rz-ghidra-build
			RESULT_VARIABLE SCRIPT_RESULT
			COMMAND \${CMAKE_COMMAND} --build . --target install
			)
		if (SCRIPT_RESULT)
			message(FATAL_ERROR \"Failed to install rz-ghidra plugin (returned \${SCRIPT_RESULT})\")
		endif()
	")
endif()

include(CPack)
