cmake_minimum_required(VERSION 3.5)
project(behavior_trees)

# https://robotics.stackexchange.com/questions/107553/nav2-how-to-create-a-custom-behavior-tree-bt-plugin-node-in-a-separate-ros2

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_pytest REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(action_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(nav2_util REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav2_core REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(nav2_behavior_tree REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wdeprecated -fPIC -Wshadow -Wnull-dereference)

include_directories(
  include
)

set(dependencies
    rosidl_default_runtime
    rclcpp
    rclcpp_action
    std_msgs
    action_msgs
    std_srvs
    nav_msgs
    nav2_util
    nav2_msgs
    nav2_core
    nav2_behavior_tree
    nav2_costmap_2d
    geometry_msgs
    tf2_ros
    sensor_msgs
    behaviortree_cpp
    msgs
    ament_index_cpp
)

# --- Generate actions FIRST ---
# rosidl_generate_interfaces(${PROJECT_NAME}
#     DEPENDENCIES builtin_interfaces geometry_msgs std_msgs action_msgs nav_msgs msgs
# )
# rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")

# ---------- common library ----------------------
# add_library(bt_common SHARED
# )
# target_include_directories(bt_common PUBLIC
#   include
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>
# )
# ament_target_dependencies(bt_common ${dependencies})
# target_link_libraries(bt_common "${cpp_typesupport_target}")
# add_dependencies(bt_common "${cpp_typesupport_target}")

# ---------- behavior tree node plugins ----------
# set(bt_node_plugin_libs
# )

# ---------- behavior tree navigator plugins ----------
# set(bt_navigator_plugin_libs
# )

# pluginlib_export_plugin_description_file(nav2_core navigator_plugins.xml)

# install(TARGETS ${bt_node_plugin_libs} ${bt_navigator_plugin_libs} bt_common
#   ARCHIVE DESTINATION lib
#   LIBRARY DESTINATION lib
#   RUNTIME DESTINATION bin
# )

# install(DIRECTORY include/
#   DESTINATION include/
# )

install(DIRECTORY behavior_trees DESTINATION share/${PROJECT_NAME})

# foreach(bt_nav_plugin ${bt_navigator_plugin_libs})
#   target_link_libraries(${bt_nav_plugin} "${cpp_typesupport_target}")
# endforeach()

# ament_export_include_directories(include)
# ament_export_libraries(${bt_node_plugin_libs} ${bt_navigator_plugin_libs} bt_common)
# ament_export_dependencies(${dependencies})
ament_package()
