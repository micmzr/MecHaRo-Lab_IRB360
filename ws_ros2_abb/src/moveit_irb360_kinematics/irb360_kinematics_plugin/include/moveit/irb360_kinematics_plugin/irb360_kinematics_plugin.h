#pragma once

// ROS2
#include <rclcpp/rclcpp.hpp>

// System
#include <memory>

// ROS msgs
#include <geometry_msgs/msg/pose.hpp>
#include <moveit_msgs/msg/kinematic_solver_info.hpp>
#include <moveit_msgs/msg/move_it_error_codes.hpp>

// MoveIt
#include <moveit/kinematics_base/kinematics_base.h>
#include <moveit/robot_state/robot_state.h>

namespace irb360_kinematics_plugin
{

class IRB360KinematicsPlugin : public kinematics::KinematicsBase
{
public:

  IRB360KinematicsPlugin();

  bool
  getPositionIK(const geometry_msgs::msg::Pose& ik_pose, const std::vector<double>& ik_seed_state,
                std::vector<double>& solution, moveit_msgs::msg::MoveItErrorCodes& error_code,
                const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
      const geometry_msgs::msg::Pose& ik_pose, const std::vector<double>& ik_seed_state, double timeout,
      std::vector<double>& solution, moveit_msgs::msg::MoveItErrorCodes& error_code,
      const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
      const geometry_msgs::msg::Pose& ik_pose, const std::vector<double>& ik_seed_state, double timeout,
      const std::vector<double>& consistency_limits, std::vector<double>& solution,
      moveit_msgs::msg::MoveItErrorCodes& error_code,
      const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
      const geometry_msgs::msg::Pose& ik_pose, const std::vector<double>& ik_seed_state, double timeout,
      std::vector<double>& solution, const IKCallbackFn& solution_callback,
      moveit_msgs::msg::MoveItErrorCodes& error_code,
      const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(
      const geometry_msgs::msg::Pose& ik_pose, const std::vector<double>& ik_seed_state, double timeout,
      const std::vector<double>& consistency_limits, std::vector<double>& solution,
      const IKCallbackFn& solution_callback, moveit_msgs::msg::MoveItErrorCodes& error_code,
      const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions()) const override;

  bool searchPositionIK(const std::vector<geometry_msgs::msg::Pose>& ik_poses, const std::vector<double>& ik_seed_state,
                        double timeout, const std::vector<double>& consistency_limits, std::vector<double>& solution,
                        const IKCallbackFn& solution_callback, moveit_msgs::msg::MoveItErrorCodes& error_code,
                        const kinematics::KinematicsQueryOptions& options = kinematics::KinematicsQueryOptions(),
                        const moveit::core::RobotState* context_state = nullptr) const override;

  bool getPositionFK(const std::vector<std::string>& link_names, const std::vector<double>& joint_angles,
                     std::vector<geometry_msgs::msg::Pose>& poses) const override;

  bool initialize(const rclcpp::Node::SharedPtr& node, const moveit::core::RobotModel& robot_model,
                  const std::string& group_name, const std::string& base_name,
                  const std::vector<std::string>& tip_frames, double search_discretization) override;

  bool supportsGroup(const moveit::core::JointModelGroup* jmg, std::string* error_text_out = nullptr) const override;                  

  /**
   * @brief  Return all the joint names in the order they are used internally
   */
  const std::vector<std::string>& getJointNames() const override;

  /**
   * @brief  Return all the link names in the order they are represented internally
   */
  const std::vector<std::string>& getLinkNames() const override;

  /**
   * @brief  Return all the variable names in the order they are represented internally
   */
  const std::vector<std::string>& getVariableNames() const;

protected:
  bool setRedundantJoints(const std::vector<unsigned int>& redundant_joint_indices) override;

private:
  bool timedOut(const rclcpp::Time& start_time, double duration) const;

  int getJointIndex(const std::string& name) const;

  bool isRedundantJoint(unsigned int index) const;

  bool active_; /** Internal variable that indicates whether solvers are configured and ready */

  moveit_msgs::msg::KinematicSolverInfo ik_group_info_; /** Stores information for the inverse kinematics solver */

  unsigned int dimension_; /** Dimension of the group */

  const moveit::core::JointModelGroup* joint_model_group_;

  moveit::core::RobotStatePtr robot_state_;

  int num_possible_redundant_joints_;

  rclcpp::Node::SharedPtr node_;

  const double sqrt3 = sqrt(3.0l);
  const double pi =  M_PI; // 3.141592653; // PI
  const double sin120 = sqrt3/2.0l; 
  const double cos120 = -0.5l; 
  const double tan60 = sqrt3;
  const double sin30 = 0.5l;
  const double tan30 = 1.l/sqrt3;

  const double e = 45.0l*2.l*sqrt3; // end effector
  const double f = 200.0l*2.l*sqrt3; // base
  const double rf = 350.0l;
  const double re = 800.0l;

  const double theta_zero = -13.2l * pi / 180.l;

  // trigonometric constants

  double x0, y0, z0; 

  const double l4_0 = 581.501; // link 4 zero postion length 
  const double z0_0 = -547.954; // z0 zero postion 

  const double phi_4_1 = atan2(0.48814,0.87276)-pi/2.l; // axis -> - pi/2

  const double alpha_0 = atan2( f/2.l/sqrt3 + rf*cos(theta_zero) - e/2.l/sqrt3 , -rf*sin(theta_zero) - z0_0 ); // for axis 1-3


  bool delta_calcAngleXZ(double x0, double y0, double z0, double &theta) const;
  bool delta_calcInverse(double x0, double y0, double z0, double &theta1, double &theta2, double &theta3) const;

};
}  
