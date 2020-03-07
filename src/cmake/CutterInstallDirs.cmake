if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Executable install directory")
elseif(APPLE)
    include(GNUInstallDirs) #TODO: use appropriate paths for macOS
else()
    include(GNUInstallDirs)
endif()
