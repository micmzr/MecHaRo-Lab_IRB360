#include <moveit/macros/class_forward.h>

#include <moveit_msgs/srv/get_position_fk.hpp>
#include <moveit_msgs/srv/get_position_ik.hpp>
#include <moveit/irb360_kinematics_plugin/irb360_kinematics_plugin.h>
#include <class_loader/class_loader.hpp>
#include <moveit/robot_state/conversions.h>
#include <iterator>

// Eigen
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <tf2/LinearMath/Matrix3x3.h>


// bug in:
// /opt/ros/humble/lib/python3.10/site-packages/moveit_configs_utils/launches.py 
// add:
// import os
// modify line with DISPLAY
//         additional_env={"DISPLAY": os.environ.get("DISPLAY")},

CLASS_LOADER_REGISTER_CLASS(irb360_kinematics_plugin::IRB360KinematicsPlugin, kinematics::KinematicsBase)

namespace irb360_kinematics_plugin
{
static const rclcpp::Logger LOGGER = rclcpp::get_logger("moveit_irb360_kinematics_plugin.irb360_kinematics_plugin");

IRB360KinematicsPlugin::IRB360KinematicsPlugin() : active_(false)
{
}

bool IRB360KinematicsPlugin::initialize(const rclcpp::Node::SharedPtr& node, const moveit::core::RobotModel& robot_model,
                                     const std::string& group_name, const std::string& base_frame,
                                     const std::vector<std::string>& tip_frames, double search_discretization)
{
  node_ = node;
  bool debug = false;

  RCLCPP_INFO(LOGGER, "IRB360KinematicsPlugin initializing");

  storeValues(robot_model, group_name, base_frame, tip_frames, search_discretization);
  joint_model_group_ = robot_model_->getJointModelGroup(group_name);
  if (!joint_model_group_)
    return false;

  if (debug)
  {
    std::cout << "Joint Model Variable Names: ------------------------------------------- \n ";
    const std::vector<std::string> jm_names = joint_model_group_->getVariableNames();
    std::copy(jm_names.begin(), jm_names.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
    std::cout << '\n';
  }

  // Get the dimension of the planning group
  dimension_ = joint_model_group_->getVariableCount();
  RCLCPP_INFO_STREAM(LOGGER, "Dimension planning group '"
                                 << group_name << "': " << dimension_
                                 << ". Active Joints Models: " << joint_model_group_->getActiveJointModels().size()
                                 << ". Mimic Joint Models: " << joint_model_group_->getMimicJointModels().size());
  
  redundant_joint_indices_.clear();
  num_possible_redundant_joints_ = 0;
  // num_possible_redundant_joints_ = (int) (joint_model_group_->getJointModels().size() - 4);

  // Copy joint names
  
  for (std::size_t i = 0; i < joint_model_group_->getJointModels().size(); ++i)
  {
    ik_group_info_.joint_names.push_back(joint_model_group_->getJointModelNames()[i]);

    RCLCPP_DEBUG(LOGGER, "IRB360KinematicsPlugin joints %s",joint_model_group_->getJointModelNames()[i].c_str());

    // if( (i == 0) || (i == 7) || (i == 14) || (i == 21) )
    //   continue;
    // else
    //   redundant_joint_indices_.push_back(i);
  }

  setRedundantJoints(redundant_joint_indices_);

  if (debug)
  {
    RCLCPP_ERROR(LOGGER, "tip links available:");
    std::copy(tip_frames_.begin(), tip_frames_.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
  }

  // Make sure all the tip links are in the link_names vector
  for (const std::string& tip_frame : tip_frames_)
  {
    if (!joint_model_group_->hasLinkModel(tip_frame))
    {
      RCLCPP_ERROR(LOGGER, "Could not find tip name '%s' in joint group '%s'", tip_frame.c_str(), group_name.c_str());
      return false;
    }
    ik_group_info_.link_names.push_back(tip_frame);
  }

  // Setup the joint state groups that we need
  robot_state_ = std::make_shared<moveit::core::RobotState>(robot_model_);
  robot_state_->setToDefaultValues();

  active_ = true;
  RCLCPP_DEBUG(LOGGER, "ROS service-based kinematics solver initialized");
  return true;
}

bool IRB360KinematicsPlugin::setRedundantJoints(const std::vector<unsigned int>& redundant_joints)
{
  RCLCPP_DEBUG(LOGGER, "ROS QQ DEBUG setRedundantJoints %ld", redundant_joints.size());

  if (num_possible_redundant_joints_ < 0)
  {
    RCLCPP_ERROR(LOGGER, "This group cannot have redundant joints");
    return false;
  }
  if (int(redundant_joints.size()) > num_possible_redundant_joints_)
  {
    RCLCPP_ERROR(LOGGER, "This group can only have %d redundant joints", num_possible_redundant_joints_);
    return false;
  }

  return true;
}

bool IRB360KinematicsPlugin::isRedundantJoint(unsigned int index) const
{
  RCLCPP_DEBUG(LOGGER, "ROS QQ DEBUG isRedundantJoint %d",index);

  for (const unsigned int& redundant_joint_indice : redundant_joint_indices_)
  {
    if (redundant_joint_indice == index)
      return true;
  }
  return false;
}

int IRB360KinematicsPlugin::getJointIndex(const std::string& name) const
{
  for (unsigned int i = 0; i < ik_group_info_.joint_names.size(); ++i)
  {
    if (ik_group_info_.joint_names[i] == name)
      return i;
  }
  return -1;
}

bool IRB360KinematicsPlugin::timedOut(const rclcpp::Time& start_time, double duration) const
{
  return ((node_->now() - start_time).seconds() >= duration);
}

bool IRB360KinematicsPlugin::getPositionIK(const geometry_msgs::msg::Pose& ik_pose,
                                        const std::vector<double>& ik_seed_state, std::vector<double>& solution,
                                        moveit_msgs::msg::MoveItErrorCodes& error_code,
                                        const kinematics::KinematicsQueryOptions& options) const
{
  std::vector<double> consistency_limits;

  return searchPositionIK(ik_pose, ik_seed_state, default_timeout_, consistency_limits, solution, IKCallbackFn(),
                          error_code, options);
}

bool IRB360KinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose& ik_pose,
                                           const std::vector<double>& ik_seed_state, double timeout,
                                           std::vector<double>& solution,
                                           moveit_msgs::msg::MoveItErrorCodes& error_code,
                                           const kinematics::KinematicsQueryOptions& options) const
{
  std::vector<double> consistency_limits;

  return searchPositionIK(ik_pose, ik_seed_state, timeout, consistency_limits, solution, IKCallbackFn(), error_code,
                          options);
}

bool IRB360KinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose& ik_pose,
                                           const std::vector<double>& ik_seed_state, double timeout,
                                           const std::vector<double>& consistency_limits, std::vector<double>& solution,
                                           moveit_msgs::msg::MoveItErrorCodes& error_code,
                                           const kinematics::KinematicsQueryOptions& options) const
{
  return searchPositionIK(ik_pose, ik_seed_state, timeout, consistency_limits, solution, IKCallbackFn(), error_code,
                          options);
}

bool IRB360KinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose& ik_pose,
                                           const std::vector<double>& ik_seed_state, double timeout,
                                           std::vector<double>& solution, const IKCallbackFn& solution_callback,
                                           moveit_msgs::msg::MoveItErrorCodes& error_code,
                                           const kinematics::KinematicsQueryOptions& options) const
{
  std::vector<double> consistency_limits;
  return searchPositionIK(ik_pose, ik_seed_state, timeout, consistency_limits, solution, solution_callback, error_code,
                          options);
}

bool IRB360KinematicsPlugin::searchPositionIK(const geometry_msgs::msg::Pose& ik_pose,
                                           const std::vector<double>& ik_seed_state, double timeout,
                                           const std::vector<double>& consistency_limits, std::vector<double>& solution,
                                           const IKCallbackFn& solution_callback,
                                           moveit_msgs::msg::MoveItErrorCodes& error_code,
                                           const kinematics::KinematicsQueryOptions& options) const
{
  // Convert single pose into a vector of one pose
  std::vector<geometry_msgs::msg::Pose> ik_poses;
  ik_poses.push_back(ik_pose);

  return searchPositionIK(ik_poses, ik_seed_state, timeout, consistency_limits, solution, solution_callback, error_code,
                          options);
}

bool IRB360KinematicsPlugin::searchPositionIK(const std::vector<geometry_msgs::msg::Pose>& ik_poses,
                                           const std::vector<double>& ik_seed_state, double /*timeout*/,
                                           const std::vector<double>& /*consistency_limits*/,
                                           std::vector<double>& solution, const IKCallbackFn& solution_callback,
                                           moveit_msgs::msg::MoveItErrorCodes& error_code,
                                           const kinematics::KinematicsQueryOptions& /*options*/,
                                           const moveit::core::RobotState* /*context_state*/) const
{
  // Check if active
  if (!active_)
  {
    RCLCPP_ERROR(LOGGER, "kinematics not active");
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }

  // Check if seed state correct
  if (ik_seed_state.size() != dimension_)
  {
    RCLCPP_ERROR_STREAM(LOGGER,
                        "Seed state must have size " << dimension_ << " instead of size " << ik_seed_state.size());
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }

  // Check that we have the same number of poses as tips
  if (tip_frames_.size() != ik_poses.size())
  {
    RCLCPP_ERROR_STREAM(LOGGER, "Mismatc hed number of pose requests (" << ik_poses.size() << ") to tip frames ("
                                                                       << tip_frames_.size()
                                                                       << ") in searchPositionIK");
    error_code.val = error_code.NO_IK_SOLUTION;
    return false;
  }

  double theta1, theta2, theta3; 

  for (std::size_t i = 0; i < tip_frames_.size(); i++)
  {

    // double x0 = ik_poses[i].position.x*1000.l;
    double x0 = ik_poses[i].position.x*1000.l+199.755l; // ABB RobotStudio version BUG? 
    double y0 = ik_poses[i].position.y*1000.l;
    // double z0 = ik_poses[i].position.z*1000.l+866.01l+z0_0; // 
    double z0 = ik_poses[i].position.z*1000.l+446.96l+z0_0+72.6240l; // ABB RobotStudio
    // x0 -199.755 y0 0 z0 -346.545
    
    RCLCPP_DEBUG(LOGGER, "x0 %g y0 %g z0 %g",x0,y0,z0);
    RCLCPP_DEBUG(LOGGER, "x %g y %g z %g",ik_poses[i].position.x,ik_poses[i].position.y,ik_poses[i].position.z);

    tf2::Quaternion q(
        ik_poses[i].orientation.x,
        ik_poses[i].orientation.y,
        ik_poses[i].orientation.z,
        ik_poses[i].orientation.w);

    tf2::Matrix3x3 mat(q);
    double roll, pitch, yaw;
    mat.getRPY(roll, pitch, yaw);

    RCLCPP_DEBUG(LOGGER, "roll %g pitch %g yaw %g",roll,pitch,yaw);

    bool res = delta_calcInverse(x0, y0, z0, theta1, theta2, theta3);

    RCLCPP_DEBUG(LOGGER, "theta1 %g theta2 %g theta3 %g",theta1,theta2,theta3);

    if(res == false)
    {
      RCLCPP_INFO(LOGGER, "NO IK SOLUTION for x %g y %g z %g",ik_poses[i].position.x, ik_poses[i].position.y, ik_poses[i].position.z);

      return false; 
    }                               

    solution.resize((i+1)*dimension_);

    // solution.at(i*dimension_ +  0) = -theta1 + theta_zero;
    // solution.at(i*dimension_ + 14) = -theta2 + theta_zero;
    // solution.at(i*dimension_ +  7) = -theta3 + theta_zero;

    // values for fix transofrmation of axis 1-3 for ABB RobotStudio zero position 
    // q = tf2::Quaternion(0.0, 0.0002067306301532129, 0.0, 0.999999978631223);
    // mat = tf2::Matrix3x3(q);
    // mat.getRPY(roll, pitch, yaw);

    // RCLCPP_INFO(LOGGER, "FIX1 roll %g pitch %g yaw %g",roll,pitch,yaw);

    // q = tf2::Quaternion(0.09921197112308286, -0.0008673005367113582, -0.005681122007539162, 0.9950497261083925);
    // mat = tf2::Matrix3x3(q);
    // mat.getRPY(roll, pitch, yaw);

    // RCLCPP_INFO(LOGGER, "FIX2 roll %g pitch %g yaw %g",roll,pitch,yaw);
    
    // q = tf2::Quaternion(-0.09921197112308286, -0.0008673005367113582, 0.005681122007539162, 0.9950497261083925);
    // mat = tf2::Matrix3x3(q);
    // mat.getRPY(roll, pitch, yaw);

    // RCLCPP_INFO(LOGGER, "FIX3 roll %g pitch %g yaw %g",roll,pitch,yaw);

    // ABB RobotStudio seems to have difrent orientation and starting postion
    
    solution.at(i*dimension_ +  0) = theta1;
    solution.at(i*dimension_ + 14) = theta2;
    solution.at(i*dimension_ +  7) = theta3;
    solution.at(i*dimension_ + 21) = yaw;

  // 4-axis

  double phi_4 = phi_4_1 - solution.at(i*dimension_ + 21);

  double d_link4 = sqrt( x0*x0 + y0*y0 + pow( z0-z0_0 - l4_0 , 2 ) ) - l4_0;  
  solution.at(i*dimension_ + 24) = d_link4/1000.l;

  double d_link_4x = sin(phi_4)*y0 + cos(phi_4)*x0; 
  double d_link_4y = -cos(phi_4)*y0 + sin(phi_4)*x0;

  solution.at(i*dimension_ + 22) = -atan2( d_link_4x , ( - z0 + z0_0 + l4_0 ) );
  solution.at(i*dimension_ + 23) = -atan2( d_link_4y , sqrt( pow(( - z0 + z0_0 + l4_0 ),2) + d_link_4x*d_link_4x ) );

  solution.at(i*dimension_ + 25) = solution.at(i*dimension_ + 23);
  solution.at(i*dimension_ + 26) = solution.at(i*dimension_ + 22);
  solution.at(i*dimension_ + 27) = -yaw;

  RCLCPP_DEBUG(LOGGER, "IRB360 FK delta link4 %g ",d_link4); 

  // 1-axis

  double alpha1 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta1) - (x0+e/2.l/sqrt3), 2 ) + y0*y0 ) , 
                        (-rf*sin(theta1) - z0) );

  solution.at(i*dimension_ + 1) = -solution.at(i*dimension_ + 0)+theta_zero; // For ABB RobotStudio
  solution.at(i*dimension_ + 2) = -atan2( y0, f/2.l/sqrt3 + rf*cos(theta1) - (x0+e/2.l/sqrt3) ); 
  solution.at(i*dimension_ + 3) = -alpha_0 + alpha1; 

  solution.at(i*dimension_ + 4) = solution.at(i*dimension_ + 1);
  solution.at(i*dimension_ + 5) = solution.at(i*dimension_ + 2);
  solution.at(i*dimension_ + 6) = solution.at(i*dimension_ + 3);

  // 2-axis

  double x0_2 = x0*cos(240.l*pi/180.l)+y0*sin(240.l*pi/180.l);
  double y0_2 = -x0*sin(240.l*pi/180.l)+y0*cos(240.l*pi/180.l);


  RCLCPP_DEBUG(LOGGER, "IRB360 Axis-3 x %g y %g",x0_2,y0_2); 

  double alpha2 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta3) - (x0_2+e/2.l/sqrt3), 2 ) + y0_2*y0_2 ) , 
                        (-rf*sin(theta3) - z0) );

  solution.at(i*dimension_ + 8) = -solution.at(i*dimension_ + 7)+theta_zero; // For ABB RobotStudio 
  solution.at(i*dimension_ + 9) = -atan2( y0_2, f/2.l/sqrt3 + rf*cos(theta3) - (x0_2+e/2.l/sqrt3) ); 
  solution.at(i*dimension_ + 10) = -alpha_0 + alpha2; 

  solution.at(i*dimension_ + 11) = solution.at(i*dimension_ + 8);
  solution.at(i*dimension_ + 12) = solution.at(i*dimension_ + 9);
  solution.at(i*dimension_ + 13) = solution.at(i*dimension_ + 10);

  // 3-axis

  double x0_3 = x0*cos(120.l*pi/180.l)+y0*sin(120.l*pi/180.l);
  double y0_3 = -x0*sin(120.l*pi/180.l)+y0*cos(120.l*pi/180.l);


  RCLCPP_DEBUG(LOGGER, "IRB360 Axis-3 x %g y %g",x0_3,y0_3); 

  double alpha3 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta2) - (x0_3+e/2.l/sqrt3), 2 ) + y0_3*y0_3 ) , 
                        (-rf*sin(theta2) - z0) );

  solution.at(i*dimension_ + 15) = -solution.at(i*dimension_ + 14)+theta_zero; // For ABB RobotStudio 
  solution.at(i*dimension_ + 16) = -atan2( y0_3, f/2.l/sqrt3 + rf*cos(theta2) - (x0_3+e/2.l/sqrt3) ); 
  solution.at(i*dimension_ + 17) = -alpha_0 + alpha3; 

  solution.at(i*dimension_ + 18) = solution.at(i*dimension_ + 15);
  solution.at(i*dimension_ + 19) = solution.at(i*dimension_ + 16);
  solution.at(i*dimension_ + 20) = solution.at(i*dimension_ + 17);

  // Run the solution callback (i.e. collision checker) if available
  if (solution_callback)
  {
    RCLCPP_DEBUG(LOGGER, "Calling solution callback on IK solution");

    solution_callback(ik_poses[i], solution, error_code);

    if (error_code.val != error_code.SUCCESS)
    {
      switch (error_code.val)
      {
        case moveit_msgs::msg::MoveItErrorCodes::FAILURE:
          RCLCPP_ERROR(LOGGER, "IK solution callback failed with with error code: FAILURE");
          break;
        case moveit_msgs::msg::MoveItErrorCodes::NO_IK_SOLUTION:
          RCLCPP_ERROR(LOGGER, "IK solution callback failed with with error code: "
                               "NO IK SOLUTION");
          break;
        default:
          RCLCPP_ERROR_STREAM(LOGGER, "IK solution callback failed with with error code: " << error_code.val);
      }
      return false;
    }
  }

  }

  RCLCPP_DEBUG(LOGGER, "IK Solver Succeeded!");

  return true;
}

bool IRB360KinematicsPlugin::supportsGroup(const moveit::core::JointModelGroup* jmg, std::string* error_text_out) const
{
  if (jmg->getName() != std::string("irb360"))
  {
    if (error_text_out)
    {
      *error_text_out = "This plugin only supports irb360 joint group";
      RCLCPP_WARN(LOGGER, "This plugin only supports irb360 joint group");
    }
    return false;
  }

  return true;
}

bool IRB360KinematicsPlugin::delta_calcAngleXZ(double x0, double y0, double z0, double &theta) const
{

  double y1 = -0.5l * tan30 * f; // f/2 * tg 30
  y0 -= 0.5l * tan30 * e; // shift center to edge
  double a = (x0 * x0 + y0 * y0 + z0 * z0 + rf * rf - re * re - y1 * y1) / (2.l * z0);
  double b = (y1 - y0) / z0;

  // discriminant
  double d = -(a + b * y1) * (a + b * y1) + rf * (b * b * rf + rf);
  if (d < 0)
  {
    RCLCPP_WARN(LOGGER, "IK solution failure: non-existing point");
    return false;              
  }

  double yj = (y1 - a * b - sqrt(d)) / (b * b + 1.l); // choosing outer point
  double zj = a + b * yj;
  theta = atan(-zj / (y1 - yj)) + ((yj > y1) ? pi : 0.l);
  
  return true;
}

bool IRB360KinematicsPlugin::delta_calcInverse(double x0, double y0, double z0, double &theta1, double &theta2, double &theta3) const
{
  theta1 = theta2 = theta3 = 0.l;
  
  bool status = delta_calcAngleXZ(y0, -x0, z0, theta1);

  double x0_2 = x0*cos120 + y0*sin120;
  double y0_2 = y0*cos120 - x0*sin120;

  if (status)
    status = delta_calcAngleXZ(y0_2, -x0_2, z0, theta2);

  double x0_3 = x0*cos120 - y0*sin120;
  double y0_3 = y0*cos120 + x0*sin120;

  if (status)
    status = delta_calcAngleXZ(y0_3, -x0_3, z0, theta3);

  // theta3 -> Axis_2 and theta2 -> axis3 - to keep with Trossen Delta tutorial

  return status;
}

bool IRB360KinematicsPlugin::getPositionFK(const std::vector<std::string>& link_names,
                                        const std::vector<double>& joint_angles,
                                        std::vector<geometry_msgs::msg::Pose>& poses) const
{
  if (!active_)
  {
    RCLCPP_ERROR(LOGGER, "kinematics not active");
    return false;
  }

  RCLCPP_ERROR(LOGGER, "FK joint_angles size: %d poses size %d links %d", 
    static_cast<int>(joint_angles.size()), static_cast<int>(poses.size()), static_cast<int>(link_names.size()));

  poses.resize(link_names.size());

  if (joint_angles.size() != dimension_)
  {
    RCLCPP_ERROR(LOGGER, "Joint angles vector must have size: %d", dimension_);
    return false;
  }

  RCLCPP_ERROR(LOGGER, "kinematics FK not implemented!!!");

  // if (!initialized_)
  // {
  //   RCLCPP_ERROR(LOGGER, "kinematics solver not initialized");
  //   return false;
  // }
  // poses.resize(link_names.size());
  // if (joint_angles.size() != dimension_)
  // {
  //   RCLCPP_ERROR(LOGGER, "Joint angles vector must have size: %d", dimension_);
  //   return false;
  // }

  // KDL::Frame p_out;
  // KDL::JntArray jnt_pos_in(dimension_);
  // jnt_pos_in.data = Eigen::Map<const Eigen::VectorXd>(joint_angles.data(), joint_angles.size());

  // bool valid = true;
  // for (unsigned int i = 0; i < poses.size(); ++i)
  // {
  //   if (fk_solver_->JntToCart(jnt_pos_in, p_out) >= 0)
  //   {
  //     poses[i] = tf2::toMsg(p_out);
  //   }
  //   else
  //   {
  //     RCLCPP_ERROR(LOGGER, "Could not compute FK for %s", link_names[i].c_str());
  //     valid = false;
  //   }
  // }
  // return valid;

  return false;
}

const std::vector<std::string>& IRB360KinematicsPlugin::getJointNames() const
{
  return ik_group_info_.joint_names;
}

const std::vector<std::string>& IRB360KinematicsPlugin::getLinkNames() const
{
  return ik_group_info_.link_names;
}

const std::vector<std::string>& IRB360KinematicsPlugin::getVariableNames() const
{
  return joint_model_group_->getVariableNames();
}

}  
