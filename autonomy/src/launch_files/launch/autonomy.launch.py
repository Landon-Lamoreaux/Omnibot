from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, OpaqueFunction, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PythonExpression, EnvironmentVariable
from ament_index_python.packages import get_package_share_directory
from launch.conditions import IfCondition
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.actions import Node
import launch_ros
from nav2_common.launch import RewrittenYaml
import os
import sys

dir_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(dir_path)

LATERAL_FOOTPRINT_BUFFER_M = 0.25
LONGITUDINAL_FOOTPRINT_BUFFER_M = 0.25


def launch_setup(context):

    simulation = EnvironmentVariable("SIMULATION")
    simulation_str = simulation.perform(context)
    print("[AUTONOMY.LAUNCH] Simulation is %s" % simulation_str)
    is_simulation = simulation_str.lower() == "true"

    pkg_share = launch_ros.substitutions.FindPackageShare(package="launch_files").find("launch_files")
    launch_dir = os.path.join(pkg_share, "launch")
    config_dir = os.path.join(pkg_share, "config")

    param_file = config_dir + "/params.yaml"

    print("[AUTONOMY LAUNCH] Parameter File at: " + str(param_file))

    # Reading the URDF file into a string variable to pass to the robot_description node.
    default_model_path = os.path.join(pkg_share, "robot_model/mecanum_robot.urdf")
    with open(default_model_path, "r") as f:
        robot_description = f.read()

    # Set controller frequency manually according to simulation speed if running the simulation.
    param_substitutions = {}
    if is_simulation:
        base_freq = 10.0
        new_freq = base_freq * int(os.environ["SIMULATION_SPEED"])
        print("[AUTONOMY_LAUNCH] Setting controller frequency to %s" % str(new_freq))
        param_substitutions.update({"controller_frequency": str(new_freq)})


    configured_params = RewrittenYaml(
        source_file=param_file, root_key="", param_rewrites=param_substitutions, convert_types=True
    )

    sim_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(launch_dir, "simulation.launch.py")),
        launch_arguments={
            "simulation": PythonExpression(str(is_simulation)),
        }.items(),
    )
    nav_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(launch_dir, "navigation.launch.py")),
        launch_arguments={
            "params_file": configured_params,
            "autostart": "True",
            "simulation": str(is_simulation),
            "config_dir": config_dir,
        }.items(),
    )

    slam_toolbox_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
        os.path.join(
            get_package_share_directory("slam_toolbox"),
            "launch",
            "online_async_launch.py",)),
        launch_arguments={
            "slam_params_file": param_file,
            "use_sim_time": str(is_simulation),
            "autostart": "true",
            "use_lifecycle_manager": "false",
        }.items(),
    )

    default_rviz_config_path = os.path.join(pkg_share, "rviz/config.rviz")
    launch_rviz = EnvironmentVariable("RVIZ")
    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", default_rviz_config_path],
        parameters=[param_file, {"use_sim_time": simulation}],
    )

    rviz_launch = TimerAction(
        period=5.0,  # Delay before starting rviz node to avoid CPU bottlenecks.
        actions=[rviz_node],
        condition=IfCondition(launch_rviz),
    )

    robot_state_publisher_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": ParameterValue(robot_description, value_type=str)}, {"use_sim_time": simulation}],
    )

    # slam_toolbox = Node(
    #     package='slam_toolbox',
    #     executable='async_slam_toolbox_node',
    #     name='slam_toolbox',
    #     output='screen',
    #     parameters=[param_file, {"use_sim_time": simulation}],
    # )

    return [
        robot_state_publisher_node,
        sim_launch,
        nav_launch,
        rviz_launch,
        slam_toolbox_launch,
    ]



def generate_launch_description():
    opfunc = OpaqueFunction(function=launch_setup)
    ld = LaunchDescription()
    ld.add_action(opfunc)
    return ld