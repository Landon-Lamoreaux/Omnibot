project(msgs)

find_package(ros_environment REQUIRED)

set(ROS_VERSION $ENV{ROS_VERSION})

set(MSG_FILES
)

set(SRV_FILES
)

set(ACTION_FILES
)

cmake_minimum_required(VERSION 3.5)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
# find_package(rosidl_default_generators REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)

# Apend "msg/" to each file name
set(TEMP_LIST "")
foreach(MSG_FILE ${MSG_FILES})
  list(APPEND TEMP_LIST "msg/${MSG_FILE}")
endforeach()
set(MSG_FILES ${TEMP_LIST})

set(TEMP_LIST "")
foreach(SRV_FILE ${SRV_FILES})
  list(APPEND TEMP_LIST "srv/${SRV_FILE}")
endforeach()
set(SRV_FILES ${TEMP_LIST})

set(TEMP_LIST "")
foreach(ACTION_FILE ${ACTION_FILES})
  list(APPEND TEMP_LIST "action/${ACTION_FILE}")
endforeach()
set(ACTION_FILES ${TEMP_LIST})

# Currently empty, will uncomment when I have message types to put in it.
# rosidl_generate_interfaces(${PROJECT_NAME}
#   ${MSG_FILES}
#   ${SRV_FILES}
#   ${ACTION_FILES}
#   DEPENDENCIES builtin_interfaces std_msgs sensor_msgs geometry_msgs nav_msgs
#   ADD_LINTER_TESTS
# )

ament_export_dependencies(rosidl_default_runtime)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
