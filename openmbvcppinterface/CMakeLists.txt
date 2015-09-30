CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12)

PROJECT(openmbvcppinterface)

### The version number
SET (openmbvcppinterface_MAJOR_VERSION 3)
SET (openmbvcppinterface_MINOR_VERSION 1)
SET (openmbvcppinterface_PATCH_VERSION 0)
set(openmbvcppinterface_VERSION ${openmbvcppinterface_MAJOR_VERSION}.${openmbvcppinterface_MINOR_VERSION}.${openmbvcppinterface_PATCH_VERSION})

#############################################
### Options #################################
#############################################

#############################################
### Search for dependencies #################
#############################################
### Use PkgConfig for searching
include(FindPkgConfig) 

### Search for hdf5serie
pkg_check_modules(HDF5SERIE REQUIRED hdf5serie>=2.0.0)
string (REPLACE ";" " " HDF5SERIE_CFLAGS_STR "${HDF5SERIE_CFLAGS}")

### Search for mbxmlutilshelper
pkg_check_modules(MBXMLUTILSHELPER REQUIRED mbxmlutilshelper>=1.3.0)
string (REPLACE ";" " " MBXMLUTILSHELPER_CFLAGS_STR "${MBXMLUTILSHELPER_CFLAGS}")

#############################################
### Include the subdirectories ##############
#############################################
add_subdirectory(openmbvcppinterface) 

#############################################
###  Installing #############################
#############################################
###  Install the pkg-config file
configure_file("openmbvcppinterface.pc.in" "openmbvcppinterface.pc" @ONLY)
install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/openmbvcppinterface.pc"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
