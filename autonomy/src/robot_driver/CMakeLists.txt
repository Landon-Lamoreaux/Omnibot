cmake_minimum_required(VERSION 3.20)
project(robot_driver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies.
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBGPIOD REQUIRED IMPORTED_TARGET libgpiod)

add_executable(robot_driver
  src/main.cpp
  src/gpio_output.cpp
  src/l298n_motor.cpp
  src/mecanum_driver.cpp
)

target_include_directories(robot_driver PRIVATE
  include
)

target_link_libraries(robot_driver
  PRIVATE PkgConfig::LIBGPIOD
  rclcpp::rclcpp
  ${geometry_msgs_TARGETS}
)

# Select the appropriate libgpiod API.
if(LIBGPIOD_VERSION VERSION_GREATER_EQUAL "2.0")
  message(STATUS "Using libgpiod ${LIBGPIOD_VERSION} (API 2.x)")
  target_compile_definitions(robot_driver
    PRIVATE MECANUM_LIBGPIOD_V2=1
  )
else()
  message(STATUS "Using libgpiod ${LIBGPIOD_VERSION} (API 1.x)")
endif()

# Install executable.
install(TARGETS robot_driver
  DESTINATION lib/${PROJECT_NAME}
)

# Install headers.
install(DIRECTORY include/
  DESTINATION include
)

ament_package()
