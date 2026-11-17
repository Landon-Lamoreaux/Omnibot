cmake_minimum_required(VERSION 3.20)
project(pointcloud_processing)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(PCL REQUIRED)
find_package(pcl_ros REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(laser_geometry REQUIRED)

include_directories(include)

add_executable(pointcloud_processing
  src/main.cpp
  src/pointcloud_processing.cpp
)

target_link_libraries(pointcloud_processing
  rclcpp::rclcpp
  laser_geometry::laser_geometry
  ${std_msgs_TARGETS}
  ${sensor_msgs_TARGETS}
  ${geometry_msgs_TARGETS}
  tf2::tf2
  tf2_ros::tf2_ros
  ${PCL_LIBRARIES}
)

install(TARGETS
  pointcloud_processing
  DESTINATION lib/${PROJECT_NAME})

ament_package()
