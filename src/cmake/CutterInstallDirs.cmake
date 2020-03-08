if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Executable install directory")
elseif(APPLE)
    include(GNUInstallDirs) #TODO: use appropriate paths for macOS
else()
    include(GNUInstallDirs)
endif()
set(ConfigPackageLocation "${CMAKE_INSTALL_LIBDIR}/Cutter" CACHE PATH "Cmake file install location")