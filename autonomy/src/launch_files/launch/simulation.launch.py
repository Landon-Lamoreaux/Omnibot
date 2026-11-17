import launch
from launch import LaunchDescription
from launch.substitutions import EnvironmentVariable
from launch.actions import OpaqueFunction
from launch.conditions import IfCondition
import launch_ros
from launch.actions import SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
import os


def launch_setup(context):

    simulation = EnvironmentVariable("SIMULATION")
    simulation_str = simulation.perform(context)

    pkg_share = launch_ros.substitutions.FindPackageShare(package="launch_files").find("launch_files")

    pkg_gazebo_ros = ""
    world_path = ""
    if simulation_str.lower() == "true":
        pkg_gazebo_ros = launch_ros.substitutions.FindPackageShare(package="ros_gz_sim").find("ros_gz_sim")
        world_path = os.path.join(pkg_share, "worlds", "sim_world.sdf")

    param_file = os.path.join(pkg_share, "config", "params.yaml")

    gz_bridge_params = os.path.join(pkg_share, "config", "gz_bridge.yaml")

    meshes_path = os.path.dirname(pkg_share)

    gazebo_ros_bridge_node = launch_ros.actions.Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=[
            "--ros-args",
            "-p",
            f"config_file:={gz_bridge_params}",
        ],
        condition=IfCondition(simulation),
        output="screen",
    )

    spawn_entity = launch_ros.actions.Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-name",
            "omni_bot",
            "-topic",
            "robot_description",
            "-z",
            "1.0",
            "-x",
            "0.0",
            "-y",
            "0.0",
        ],
        output="screen",
        condition=IfCondition(simulation),
    )

    robot_localization_local = launch_ros.actions.Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_filter_node_odom",
        output="screen",
        parameters=[param_file, {"use_sim_time": simulation}],
        remappings=[("odometry/filtered", "odometry/local")],
        condition=IfCondition(simulation),
    )
    robot_localization_global = launch_ros.actions.Node(
        package="robot_localization",
        executable="ekf_node",
        name="ekf_filter_node_map",
        output="screen",
        parameters=[param_file, {"use_sim_time": simulation}],
        remappings=[("odometry/filtered", "odometry/global")],
        condition=IfCondition(simulation),
    )

    launch_gazebo_server = launch.actions.IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(pkg_gazebo_ros, "launch", "gz_sim.launch.py")),
        condition=IfCondition(simulation),
        launch_arguments={
            "gz_args": ["-r -s -v4 --headless-rendering ", world_path],  # -v4 for verbose logging.
            "on_exit_shutdown": "true",
        }.items(),
        # -r = start running immediately; -s = server only; -v4 = logging verbosity
    )
    launch_gazebo_gui = launch.actions.IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(pkg_gazebo_ros, "launch", "gz_sim.launch.py")),
        condition=IfCondition(simulation),
        launch_arguments={"gz_args": ["-g -v4 ", world_path]}.items(),
        # -g = client only; -v4 = logging verbosity
    )

    set_gazebo_model_path = SetEnvironmentVariable(name='GZ_SIM_RESOURCE_PATH',
                                                   value=[meshes_path, os.pathsep, os.environ.get('GZ_SIM_RESOURCE_PATH', '')])

    return [
        launch_gazebo_server,
        gazebo_ros_bridge_node,
        spawn_entity,
        robot_localization_local,
        robot_localization_global,
        # launch_gazebo_gui,
        set_gazebo_model_path,
    ]


def generate_launch_description():
    opfunc = OpaqueFunction(function=launch_setup)
    ld = LaunchDescription()
    ld.add_action(opfunc)
    return ld
