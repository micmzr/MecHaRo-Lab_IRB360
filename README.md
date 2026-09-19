Unofficial delta ABB IRB360 ROS2/MoveIT driver. 
For limited use only with system installed in MecHaRo-Lab at Technical University of Gdańsk.  

**PC**

Everything presented here is mean to be run on the PC.
 
_Install_

``` bash
git clone --recursive -b lyrical https://github.com/micmzr/MecHaRo-Lab_IRB360.git
cd MecHaRo-Lab_IRB360/ws_ros2_abb
rosdep install --ignore-src --from-path src/ -y --rosdistro $ROS_DISTRO
colcon build
source install/setup.bash
```

**_LOOPBACK Simulation_**

Generally each command should be run in separate terminal tab. Please remember to run source command. 

_Control_
``` bash
ros2 launch irb360 abb_control.launch.py runtime_config_package:=irb360 description_package:=irb360 description_file:=irb360.xacro launch_rviz:=false moveit_config_package:=irb360 use_fake_hardware:=true
```
_MoveIT_
``` bash
ros2 launch irb360 abb_moveit.launch.py robot_xacro_file:=irb360.xacro support_package:=irb360 moveit_config_package:=irb360 moveit_config_file:=abb_irb360.srdf.xacro
```

**_RobotStudio_ - IRB360 with EGM required - see IRB360_01.rspag and docs.**

_Control_
``` bash
ros2 launch irb360 abb_control.launch.py runtime_config_package:=irb360 description_package:=irb360 description_file:=irb360.xacro launch_rviz:=false moveit_config_package:=irb360 use_fake_hardware:=false rws_ip:=X.X.X.X
```
_MoveIT_
``` bash
ros2 launch irb360 abb_moveit.launch.py robot_xacro_file:=irb360.xacro support_package:=irb360 moveit_config_package:=irb360 moveit_config_file:=abb_irb360.srdf.xacro
```

