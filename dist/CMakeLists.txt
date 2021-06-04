set(CUTTER_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../cutter-deps")
unset(RZ_GHIDRA_PREFIX_PATH)

################################################
# Windows
################################################

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
	set(RIZIN_INSTALL_PLUGDIR "lib/plugins")

	if (CUTTER_PACKAGE_DEPENDENCIES)
		if (CUTTER_ENABLE_PYTHON)
			if (CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
				set(CPACK_INSTALL_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/WindowsBundlePython.cmake)
			endif()
			find_package(PythonInterp REQUIRED)
			install(DIRECTORY ${CUTTER_DEPS}/pyside/lib/site-packages DESTINATION "python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
			install(FILES ${CUTTER_DEPS}/pyside/bin/shiboken2.abi3.dll ${CUTTER_DEPS}/pyside/bin/pyside2.abi3.dll DESTINATION .)
		endif()
		install(SCRIPT WindowsBundleQt.cmake)
	endif()
	if (CUTTER_PACKAGE_JSDEC AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			set(ENV{RZ_PREFIX} \"\${CMAKE_INSTALL_PREFIX}\")
			set(ENV{PATH} \"\${CMAKE_INSTALL_PREFIX};\$ENV{PATH}\")
			execute_process(COMMAND powershell \"${CMAKE_CURRENT_SOURCE_DIR}/bundle_jsdec.ps1\" \"\${CMAKE_INSTALL_PREFIX}\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package jsdec\")
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
	if (CUTTER_ENABLE_CRASH_REPORTS)
		list(APPEND CPACK_BUNDLE_APPLE_CODESIGN_FILES "/Contents/Frameworks/Breakpad.framework/Versions/Current/Resources/breakpadUtilities.dylib")
	endif()


	find_program(MACDEPLOYQT_PATH macdeployqt HINTS "${Qt5_DIR}/../../../bin")
	if(NOT MACDEPLOYQT_PATH)
		message(FATAL_ERROR "Failed to find macdeployqt")
	endif()

	unset(ADJUST_RIZIN_LIBS)
	foreach(_lib ${RZ_LIBS})
		list(APPEND ADJUST_RIZIN_LIBS "${RIZIN_INSTALL_DIR}/lib/lib${_lib}.dylib")
	endforeach()

	if(CUTTER_PACKAGE_DEPENDENCIES AND CUTTER_ENABLE_PYTHON)
		set(EMBED_PYTHON_SH "${CMAKE_CURRENT_SOURCE_DIR}/appbundle_embed_python.sh")
		set(PYTHON_FRAMEWORK_DIR "${CUTTER_DEPS}/python/Python.framework")
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
	set(RIZIN_INSTALL_PLUGDIR "share/rizin/plugins") # escaped backslash on purpose, should be evaluated later

	if (CUTTER_PACKAGE_JSDEC AND CUTTER_ENABLE_DEPENDENCY_DOWNLOADS)
		install(CODE "
			execute_process(COMMAND \"${CMAKE_CURRENT_SOURCE_DIR}/../scripts/jsdec.sh\" --prefix=\${CMAKE_INSTALL_PREFIX}
								\"-Drizin_incdir=\${CMAKE_INSTALL_PREFIX}/include/librz\"
								\"-Drizin_libdir=\${CMAKE_INSTALL_PREFIX}/lib\"
							WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
							RESULT_VARIABLE SCRIPT_RESULT)
			if (SCRIPT_RESULT)
				message(FATAL_ERROR \"Failed to package jsdec\")
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
			#GIT_TAG v0.2.0
			GIT_TAG dev
			#GIT_SHALLOW ON # disable this line when using commit hash
			CONFIGURE_COMMAND ""
			BUILD_COMMAND ""
			INSTALL_COMMAND ""
		)
	endif()
	if (WIN32)
		set (GHIDRA_OPTIONS "
			\"-DRIZIN_INSTALL_PLUGDIR=\${CMAKE_INSTALL_PREFIX}/${RIZIN_INSTALL_PLUGDIR}\"
			-DCUTTER_INSTALL_PLUGDIR=plugins/native")
	elseif (APPLE)
		set (GHIDRA_OPTIONS "
			\"-DRIZIN_INSTALL_PLUGDIR=\${CMAKE_INSTALL_PREFIX}/${RIZIN_INSTALL_PLUGDIR}\"
			-DCUTTER_INSTALL_PLUGDIR=plugins/native")
	else()
		set (GHIDRA_OPTIONS "")
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
			message(FATAL_ERROR \"Failed to configure rz-ghidra\")
		endif()
		execute_process(WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rz-ghidra-prefix/src/rz-ghidra-build
			RESULT_VARIABLE SCRIPT_RESULT
			COMMAND \${CMAKE_COMMAND} --build . --target install
			)
		if (SCRIPT_RESULT)
			message(FATAL_ERROR \"Failed to install rz-ghidra plugin\")
		endif()
	")
endif()

include(CPack)
