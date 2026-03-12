#include "irb360_fk_joints_publisher/irb360_fk_joints_publisher.hpp"

IRB360FKPublisher::IRB360FKPublisher() : rclcpp::Node("irb360_fk_joints_publisher")
{
  auto subscriber_options = rclcpp::SubscriptionOptions();
  auto qos = rclcpp::QoS(1);

  joint_state_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
    "joint_states",
    rclcpp::SensorDataQoS(),
    std::bind(&IRB360FKPublisher::callbackJointState, this, std::placeholders::_1),
    subscriber_options);

  joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("joint_states_irb360",qos);

  RCLCPP_INFO(this->get_logger(), "IRB360 FK joints publisher initialised");
}

IRB360FKPublisher::~IRB360FKPublisher()
{

  for (int i = 0; i < joints_cnt; i++)
  {
    RCLCPP_INFO(this->get_logger(), "Joints %d name %s",i,joints.name[i].c_str());
  }

  RCLCPP_INFO(this->get_logger(), "IRB360 FK joints publisher terminated");
}

void IRB360FKPublisher::callbackJointState(
  const sensor_msgs::msg::JointState::ConstSharedPtr state)
{
  if (state->name.size() != state->position.size())
  {
    if (state->position.empty())
    {
      RCLCPP_WARN(
          this->get_logger(), "IRB360 FK ignored a JointState message about joint(s) "
                              "\"%s\"(,...) whose position member was empty.",
          state->name[0].c_str());
    }
    else
    {
      RCLCPP_ERROR(this->get_logger(), "IRB360 FK ignored an invalid JointState message");
    }
    return;
  }

  state_cnt = state->name.size();

  joints.name.resize(joints_cnt);
  joints.position.resize(joints_cnt);

  joints.header = state->header;

  if (joints_cnt == state_cnt)
  {
    for (int i = 0; i < joints_cnt; i++)
    {
      joints.name[i] = state->name[i];
      joints.position[i] = state->position[i];
    }
  }
  else if(state_cnt == 4)
  {
    for (int i = 0; i < joints_cnt; i++)
    {
      joints.position[i] = 0.l;
    }

    // It should be done in more elegant way - TODO 

    joints.name[0] = "joint_1";
    joints.name[1] = "joint_1_1_1";
    joints.name[2] = "joint_1_1_2";
    joints.name[3] = "joint_1_1_3";
    joints.name[4] = "joint_1_2_1";
    joints.name[5] = "joint_1_2_2";
    joints.name[6] = "joint_1_2_3";
    joints.name[7] = "joint_2";
    joints.name[8] = "joint_2_1_1";
    joints.name[9] = "joint_2_1_2";
    joints.name[10] = "joint_2_1_3";
    joints.name[11] = "joint_2_2_1";
    joints.name[12] = "joint_2_2_2";
    joints.name[13] = "joint_2_2_3";
    joints.name[14] = "joint_3";
    joints.name[15] = "joint_3_1_1";
    joints.name[16] = "joint_3_1_2";
    joints.name[17] = "joint_3_1_3";
    joints.name[18] = "joint_3_2_1";
    joints.name[19] = "joint_3_2_2";
    joints.name[20] = "joint_3_2_3";
    joints.name[21] = "joint_4";
    joints.name[22] = "joint_4_2";
    joints.name[23] = "joint_4_3";
    joints.name[24] = "joint_4_4";
    joints.name[25] = "joint_4_5";
    joints.name[26] = "joint_4_6";
    joints.name[27] = "joint_4_7";
  }

  if (state_cnt == joints_cnt)
  {
    // Axis - 1
    joints.position[0] = state->position[0];
    // Axis - 2
    joints.position[7] = state->position[7];
    // Axis - 3
    joints.position[14] = state->position[14];
    // Axis - 4
    joints.position[21] = state->position[21];
  }
  else if(state_cnt == 4)
  {
    // Axis - 1
    joints.position[0] = state->position[0];
    // Axis - 2
    joints.position[7] = state->position[1];
    // Axis - 3
    joints.position[14] = state->position[2];
    // Axis - 4
    joints.position[21] = state->position[3];
  }
  else
  {
    RCLCPP_WARN(this->get_logger(), "IRB360 FK not supported number of joints %d",state_cnt);
  }

  if ( FK() )
  {
    RCLCPP_DEBUG(this->get_logger(), "IRB360 FK x0 %g y0 %g z0 %g",x0,y0,z0); 
  }
  else 
  {
    RCLCPP_WARN(this->get_logger(), "IRB360 FK not existing point theta1 %g theta2 %g theta3 %g theta_zero %g"
    ,joints.position[7],joints.position[14],joints.position[21],theta_zero); 
  }

  joint_state_pub_->publish(joints);

}

bool IRB360FKPublisher::FK(void)
{
  double t = (f - e) * tan30 / 2.l;

  // double theta1 = -joints.position[0]+theta_zero;
  // double theta2 = -joints.position[14]+theta_zero;
  // double theta3 = -joints.position[7]+theta_zero; // reverse order of joints nummbering to the Trossen Delta tutorial

  // ABB RobotStudio version - diffrent zero state then in the cad drawnings 
  double theta1 = joints.position[0];
  double theta2 = joints.position[14];
  double theta3 = joints.position[7]; // reverse order of joints nummbering to the Trossen Delta tutorial

  double y1 = -(t + rf * cos(theta1));
  double z1 = -rf * sin(theta1);
  double y2 = (t + rf * cos(theta2)) * sin30;
  double x2 = y2 * tan60;
  double z2 = -rf * sin(theta2);
  double y3 = (t + rf * cos(theta3)) * sin30;
  double x3 = -y3 * tan60;
  double z3 = -rf * sin(theta3);
  double dnm = (y2 - y1) * x3 - (y3 - y1) * x2;
  double w1 = y1 * y1 + z1 * z1;
  double w2 = x2 * x2 + y2 * y2 + z2 * z2;
  double w3 = x3 * x3 + y3 * y3 + z3 * z3;

  // x = (a1*z + b1)/dnm
  double a1 = (z2 - z1) * (y3 - y1) - (z3 - z1) * (y2 - y1);
  double b1 = -((w2 - w1) * (y3 - y1) - (w3 - w1) * (y2 - y1)) / 2.l;
  // y = (a2*z + b2)/dnm;
  double a2 = -(z2 - z1) * x3 + (z3 - z1) * x2;
  double b2 = ((w2 - w1) * x3 - (w3 - w1) * x2) / 2.l;
  // a*z^2 + b*z + c = 0
  double a = a1 * a1 + a2 * a2 + dnm * dnm;
  double b = 2 * (a1 * b1 + a2 * (b2 - y1 * dnm) - z1 * dnm * dnm);
  double c = (b2 - y1 * dnm) * (b2 - y1 * dnm) + b1 * b1 + dnm * dnm * (z1 * z1 - re * re);

  // discriminant
  double d = b * b - 4.l * a * c;
  
  if (d < 0)
    return false; // non-existing point

  z0 = -0.5l * (b + sqrt(d)) / a; // real z0 is lower -(275+42) - base and faceplate - -547.954 - zero position 
  // x0 = (a1 * z0 + b1) / dnm;
  // y0 = (a2 * z0 + b2) / dnm; // orginal version from Trossen Delta tutorial 
  y0 = (a1 * z0 + b1) / dnm;
  x0 = -(a2 * z0 + b2) / dnm; // coordinates transformation x_org -> y, -y_org -> x 

  // 4-axis 

  double phi_4 = phi_4_1 - joints.position[21];

  double d_link4 = sqrt( x0*x0 + y0*y0 + pow( z0-z0_0 - l4_0 , 2 ) ) - l4_0;  
  joints.position[24] = d_link4/1000.l;

  double d_link_4x = sin(phi_4)*y0 + cos(phi_4)*x0; 
  double d_link_4y = -cos(phi_4)*y0 + sin(phi_4)*x0;

  joints.position[22] = -atan2( d_link_4x , ( - z0 + z0_0 + l4_0 ) );
  joints.position[23] = -atan2( d_link_4y , sqrt( pow(( - z0 + z0_0 + l4_0 ),2) + d_link_4x*d_link_4x ) );

  joints.position[25] = joints.position[23];
  joints.position[26] = joints.position[22];
  joints.position[27] = -joints.position[21];

  RCLCPP_DEBUG(this->get_logger(), "IRB360 FK delta link4 %g ",d_link4); 

  // 1-axis

  double alpha1 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta1) - (x0+e/2.l/sqrt3), 2 ) + y0*y0 ) , 
                        (-rf*sin(theta1) - z0) );

  joints.position[1] = -joints.position[0]+theta_zero; 
  joints.position[2] = -atan2( y0, f/2.l/sqrt3 + rf*cos(theta1) - (x0+e/2.l/sqrt3) ); 
  joints.position[3] = -alpha_0 + alpha1; 

  joints.position[4] = joints.position[1];
  joints.position[5] = joints.position[2];
  joints.position[6] = joints.position[3];

  // 2-axis

  double x0_2 = x0*cos(240.l*pi/180.l)+y0*sin(240.l*pi/180.l);
  double y0_2 = -x0*sin(240.l*pi/180.l)+y0*cos(240.l*pi/180.l);


  RCLCPP_DEBUG(this->get_logger(), "IRB360 Axis-3 x %g y %g",x0_2,y0_2); 

  double alpha2 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta3) - (x0_2+e/2.l/sqrt3), 2 ) + y0_2*y0_2 ) , 
                        (-rf*sin(theta3) - z0) );

  joints.position[8] = -joints.position[7]+theta_zero; 
  joints.position[9] = -atan2( y0_2, f/2.l/sqrt3 + rf*cos(theta3) - (x0_2+e/2.l/sqrt3) ); 
  joints.position[10] = -alpha_0 + alpha2; 

  joints.position[11] = joints.position[8];
  joints.position[12] = joints.position[9];
  joints.position[13] = joints.position[10];

  // 3-axis

  double x0_3 = x0*cos(120.l*pi/180.l)+y0*sin(120.l*pi/180.l);
  double y0_3 = -x0*sin(120.l*pi/180.l)+y0*cos(120.l*pi/180.l);


  RCLCPP_DEBUG(this->get_logger(), "IRB360 Axis-3 x %g y %g",x0_3,y0_3); 

  double alpha3 = atan2( sqrt( pow( f/2.l/sqrt3 + rf*cos(theta2) - (x0_3+e/2.l/sqrt3), 2 ) + y0_3*y0_3 ) , 
                        (-rf*sin(theta2) - z0) );

  joints.position[15] = -joints.position[14]+theta_zero; 
  joints.position[16] = -atan2( y0_3, f/2.l/sqrt3 + rf*cos(theta2) - (x0_3+e/2.l/sqrt3) ); 
  joints.position[17] = -alpha_0 + alpha3; 

  joints.position[18] = joints.position[15];
  joints.position[19] = joints.position[16];
  joints.position[20] = joints.position[17];

  return true; 
}

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);

  rclcpp::executors::SingleThreadedExecutor executor;
  rclcpp::Node::SharedPtr IRB360FK = std::make_shared<IRB360FKPublisher>();
  executor.add_node(IRB360FK);
  executor.spin();

  rclcpp::shutdown();

  return 0;
}