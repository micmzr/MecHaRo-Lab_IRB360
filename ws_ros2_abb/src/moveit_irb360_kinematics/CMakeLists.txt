cmake_minimum_required(VERSION 3.18)
project(moveit_irb360_kinematics LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Common cmake code applied to all moveit packages
find_package(moveit_common REQUIRED)
moveit_package()

find_package(ament_cmake REQUIRED)

find_package(Eigen3 REQUIRED)
find_package(moveit_core REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(moveit_ros_planning REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2_kdl REQUIRED)

set(THIS_PACKAGE_INCLUDE_DIRS
  irb360_kinematics_plugin/include
)

set(THIS_PACKAGE_LIBRARIES
  moveit_irb360_kinematics_plugin
)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  Eigen3
  moveit_core
  moveit_msgs
  moveit_ros_planning
  pluginlib
  rclcpp
)

pluginlib_export_plugin_description_file(moveit_core irb360_kinematics_plugin_description.xml)

include_directories(${THIS_PACKAGE_INCLUDE_DIRS})

add_subdirectory(irb360_kinematics_plugin)

install(
  TARGETS ${THIS_PACKAGE_LIBRARIES}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include/moveit_irb360_kinematics
)

ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})

ament_package()
