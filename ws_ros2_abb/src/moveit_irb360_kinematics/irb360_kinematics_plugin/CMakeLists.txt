add_library(moveit_irb360_kinematics_plugin SHARED src/irb360_kinematics_plugin.cpp)
set_target_properties(moveit_irb360_kinematics_plugin PROPERTIES VERSION "${${PROJECT_NAME}_VERSION}")

ament_target_dependencies(moveit_irb360_kinematics_plugin
  rclcpp
  moveit_core
  moveit_msgs
)

install(DIRECTORY include/ DESTINATION include/moveit_irb360_kinematics)
