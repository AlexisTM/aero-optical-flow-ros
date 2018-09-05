cmake_minimum_required(VERSION 2.8.3)
project(aero_optical_flow)
add_compile_options(-std=c++11)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

find_package(catkin REQUIRED COMPONENTS
  diagnostic_msgs
  roscpp
  sensor_msgs
  image_transport
  camera_info_manager
)

set(MAVLINK_DIALECT common)

add_subdirectory(modules/OpticalFlow)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## INCLUDE_DIRS: uncomment this if your package contains header files
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES intel-aero
   CATKIN_DEPENDS diagnostic_msgs roscpp sensor_msgs image_transport camera_info_manager
#  DEPENDS system_lib
)

###########
## Build ##
###########

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)


add_executable(aero-optical-flow src/mainloop.cpp src/camera.cpp src/mavlink_tcp.cpp src/main.cpp)
set_property(TARGET aero-optical-flow PROPERTY CXX_STANDARD 11)

add_dependencies(aero-optical-flow ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
set_property(TARGET aero-optical-flow PROPERTY COMPILE_FLAGS "-Wall -Wextra -Wundef -Wformat=2 -Wlogical-op -Wsign-compare -Wformat-security -Wformat-nonliteral -Wpointer-arith -Winit-self -Wmissing-noreturn -Wstrict-aliasing=2 -Wno-overlength-strings -Wno-missing-field-initializers -Wuninitialized")
target_include_directories(aero-optical-flow PUBLIC ${CMAKE_SOURCE_DIR}/modules/mavlink_c/${MAVLINK_DIALECT})
target_link_libraries(aero-optical-flow ${OpenCV_LIBS})
target_link_libraries(aero-optical-flow OpticalFlow)
target_link_libraries(aero-optical-flow pthread)
target_link_libraries(aero-optical-flow
   ${catkin_LIBRARIES}
)



## Declare a C++ library
# add_library(${PROJECT_NAME}
#   src/${PROJECT_NAME}/intel-aero.cpp
# )

## Add cmake target dependencies of the library
## as an example, code may need to be generated before libraries
## either from message generation or dynamic reconfigure
# add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Declare a C++ executable
## With catkin_make all packages are built within a single CMake context
## The recommended prefix ensures that target names across packages don't collide
# add_executable(${PROJECT_NAME}_node src/intel-aero_node.cpp)

## Rename C++ executable without prefix
## The above recommended prefix causes long target names, the following renames the
## target back to the shorter version for ease of user use
## e.g. "rosrun someones_pkg node" instead of "rosrun someones_pkg someones_pkg_node"
# set_target_properties(${PROJECT_NAME}_node PROPERTIES OUTPUT_NAME node PREFIX "")

## Add cmake target dependencies of the executable
## same as for the library above
# add_dependencies(${PROJECT_NAME}_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Specify libraries to link a library or executable target against
# target_link_libraries(${PROJECT_NAME}_node
#   ${catkin_LIBRARIES}
# )

#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executable scripts (Python etc.) for installation
## in contrast to setup.py, you can choose the destination
# install(PROGRAMS
#   scripts/my_python_script
#   DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark executables and/or libraries for installation
# install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_node
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
# )

## Mark cpp header files for installation
# install(DIRECTORY include/${PROJECT_NAME}/
#   DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
#   FILES_MATCHING PATTERN "*.h"
#   PATTERN ".svn" EXCLUDE
# )

## Mark other files for installation (e.g. launch and bag files, etc.)
# install(FILES
#   # myfile1
#   # myfile2
#   DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
# )

#############
## Testing ##
#############

## Add gtest based cpp test target and link libraries
# catkin_add_gtest(${PROJECT_NAME}-test test/test_intel-aero.cpp)
# if(TARGET ${PROJECT_NAME}-test)
#   target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME})
# endif()

## Add folders to be run by python nosetests
# catkin_add_nosetests(test)
configure_file(aero-optical-flow.service.in aero-optical-flow.service)

install(TARGETS aero-optical-flow DESTINATION bin)
