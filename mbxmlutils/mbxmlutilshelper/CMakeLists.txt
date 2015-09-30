#############################################
### Include the subdirectories
#############################################
# add the project library with the sources
add_library(mbxmlutilsbase SHARED
last_write_time.cc
shared_library.cc
)
target_link_libraries(mbxmlutilsbase)

add_library(mbxmlutilshelper SHARED 
dom.cc 
getinstallpath.cc 
utils.cc
)
    
target_link_libraries(mbxmlutilshelper mbxmlutilsbase)

# Create a config.h due to the checks made before
configure_file ("${PROJECT_SOURCE_DIR}/mbxmlutilshelper/config.h.in"
                "${PROJECT_BINARY_DIR}/config.h" )
# Include the path when compiling
include_directories(${PROJECT_BINARY_DIR})

#############################################
### Install
#############################################

# Install the library fmatvec
install(TARGETS mbxmlutilsbase mbxmlutilshelper 
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)

# Install the library-headers
install(FILES last_write_time.h 
        shared_library.h 
        getsharedlibpath.h 
        getsharedlibpath_impl.h 
        dom.h 
        getinstallpath.h 
        utils.h 
        casadiXML.h        
  DESTINATION include/mbxmlutilshelper
        )

