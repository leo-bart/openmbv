CMAKE_MINIMUM_REQUIRED (VERSION 2.8.12)

PROJECT(mbxmlutils)

### The version number
SET (mbxmlutils_MAJOR_VERSION 1)
SET (mbxmlutils_MINOR_VERSION 3)
SET (mbxmlutils_PATCH_VERSION 0)
set(mbxmlutils_VERSION ${mbxmlutils_MAJOR_VERSION}.${mbxmlutils_MINOR_VERSION}.${mbxmlutils_PATCH_VERSION})

#############################################
### Options #################################
#############################################

#############################################
### Search for dependencies #################
#############################################
### Use PkgConfig for searching
include(FindPkgConfig) 

### Serach for casadi
pkg_check_modules(CASADI REQUIRED casadi>=2.0.0)

### Search for fmatvec
pkg_check_modules(FMATVEC REQUIRED fmatvec>=2.0.0)
include_directories(${FMATVEC_INCLUDE_DIRS})

### Check for the boost dependency
find_package(Boost REQUIRED COMPONENTS system filesystem) 
include_directories(${Boost_INCLUDE_DIRS})

set(LIBDL "-lboost_system -lboost_filesystem")

### Check for the xercesc dependency
find_library(XercesLibrary NAMES xerces-c REQUIRED) 
include_directories(${XercesLibrary_INCLUDE_DIRS})

#############################################
### Include the subdirectories ##############
#############################################
add_subdirectory(mbxmlutilshelper) 

#############################################
###  Installing #############################
#############################################
###  Install the pkg-config files
configure_file("mbxmlutilsbase.pc.in" "mbxmlutilsbase.pc" @ONLY)
install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/mbxmlutilsbase.pc"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")

configure_file("mbxmlutilshelper.pc.in" "mbxmlutilshelper.pc" @ONLY)
install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/mbxmlutilshelper.pc"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
