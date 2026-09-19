# generated from rosidl_cmake/cmake/rosidl_cmake_aggregate_target-extras.cmake.in

# Create a convenience aggregate target abb_robot_msgs::abb_robot_msgs
# that links all generated interface targets, so downstream packages can use
# a single modern CMake target name instead of ${abb_robot_msgs_TARGETS}.
if(abb_robot_msgs_TARGETS AND NOT TARGET abb_robot_msgs::abb_robot_msgs)
  add_library(abb_robot_msgs::abb_robot_msgs INTERFACE IMPORTED)
  set_target_properties(abb_robot_msgs::abb_robot_msgs PROPERTIES
    INTERFACE_LINK_LIBRARIES "${abb_robot_msgs_TARGETS}")
endif()
