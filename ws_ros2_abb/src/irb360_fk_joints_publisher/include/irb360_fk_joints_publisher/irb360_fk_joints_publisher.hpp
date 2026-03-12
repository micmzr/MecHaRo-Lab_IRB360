
#ifndef IRB360_FK_PUBLISHER__IRB360_FK_PUBLISHER_HPP_
#define IRB360_FK_PUBLISHER__IRB360_FK_PUBLISHER_HPP_


#define _USE_MATH_DEFINES
#include <cmath>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

class IRB360FKPublisher : public rclcpp::Node
{
public:  
  explicit IRB360FKPublisher();
  ~IRB360FKPublisher();

protected:
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
  rclcpp::Time last_callback_time_;

  void callbackJointState(const sensor_msgs::msg::JointState::ConstSharedPtr state);
  bool FK(void);

  sensor_msgs::msg::JointState joints;

  const int joints_cnt = 28;
  int state_cnt;

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

};

#endif  // IRB360_FK_PUBLISHER__IRB360_FK_PUBLISHER_HPP_
