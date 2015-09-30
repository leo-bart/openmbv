#############################################
### Include the subdirectories
#############################################
# add the project library with the sources
add_library(openmbvcppinterface SHARED
  object.cc
  objectfactory.cc
  body.cc
  dynamiccoloredbody.cc
  group.cc
  rigidbody.cc
  compoundrigidbody.cc
  frame.cc
  grid.cc
  frustum.cc
  sphere.cc
  extrusion.cc
  rotation.cc
  invisiblebody.cc
  polygonpoint.cc
  arrow.cc
  path.cc
  coilspring.cc
  cube.cc
  ivbody.cc
  cuboid.cc
  spineextrusion.cc
  nurbsdisk.cc
)

SET_TARGET_PROPERTIES(openmbvcppinterface PROPERTIES COMPILE_FLAGS "${HDF5SERIE_CFLAGS_STR}")
target_link_libraries(openmbvcppinterface "${HDF5SERIE_LDFLAGS}")

# Create a config.h due to the checks made before
configure_file ("${PROJECT_SOURCE_DIR}/openmbvcppinterface/config.h.in"
                "${PROJECT_BINARY_DIR}/config.h" )
# Include the path when compiling
include_directories(${PROJECT_BINARY_DIR})
include_directories("${CMAKE_SOURCE_DIR}")

#############################################
### Install
#############################################

# Install the library openmbvcppinterface
install(TARGETS openmbvcppinterface
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)

# Install the library-headers
install(FILES
        object.h
        objectfactory.h
        body.h
        dynamiccoloredbody.h
        group.h
        rigidbody.h
        rigidbodycos.png
        rigidbodycos.eps
        compoundrigidbody.h
        frame.h
        grid.h
        frustum.h
        sphere.h
        extrusion.h
        rotation.h
        invisiblebody.h
        polygonpoint.h
        arrow.h
        path.h
        coilspring.h
        cube.h
        ivbody.h
        cuboid.h
        spineextrusion.h
        nurbsdisk.h
        DESTINATION include/openmbvcppinterface
        )

