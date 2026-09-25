cmake_minimum_required(VERSION 3.30)
project(irb360_fk_joints_publisher)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra)
endif()
if(CMAKE_COMPILER_IS_MSVC)
  add_compile_options(/W1)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED) 

include_directories(
  include
)

set(THIS_PACKAGE_INCLUDE_DEPENDS
  rclcpp
  sensor_msgs
  )

add_executable(irb360_fk_joints_publisher src/irb360_fk_joints_publisher.cpp)
target_link_libraries(
  irb360_fk_joints_publisher
  PUBLIC
  rclcpp::rclcpp
  sensor_msgs::sensor_msgs
)

install(TARGETS irb360_fk_joints_publisher
  DESTINATION lib/${PROJECT_NAME}
)

ament_export_dependencies(${THIS_PACKAGE_INCLUDE_DEPENDS})
ament_package()
