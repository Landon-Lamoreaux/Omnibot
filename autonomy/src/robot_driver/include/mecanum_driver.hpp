
    
  
#ifndef ROBOT_DRIVER__MECANUM_MOTOR_DRIVER_HPP_
#define ROBOT_DRIVER__MECANUM_MOTOR_DRIVER_HPP_

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "geometry_msgs/msg/twist_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "l298n_motor.hpp"

namespace robot_driver
{

/**
 * ROS 2 GPIO-only mecanum motor driver for two L298N modules.
 *
 * Subscribes to TwistStamped, calculates four mecanum wheel commands, and
 * drives each L298N channel using two direction GPIOs plus one digital enable.
 * The ENA/ENB outputs are ON/OFF, not PWM; enabled motors receive full drive.
 */
class RobotDriver final : public rclcpp::Node
{
public:
  explicit RobotDriver(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~RobotDriver() override;

private:
  using SteadyClock = std::chrono::steady_clock;

  void loadParametersAndInitializeHardware();
  void commandCallback(const geometry_msgs::msg::TwistStamped::ConstSharedPtr msg);
  void updateHardware();
  void stopAll() noexcept;
  void validateFourEntries(const std::vector<std::string> & values, const std::string & name) const;
  void validateFourEntries(const std::vector<std::int64_t> & values, const std::string & name) const;

  bool hardware_enabled_{false};
  bool command_received_{false};
  bool timed_out_{false};
  bool hardware_faulted_{false};

  double wheel_radius_{0.12};
  double half_wheelbase_{0.35};
  double half_track_{0.27};
  double command_timeout_sec_{0.30};
  double update_rate_hz_{50.0};
  double enable_threshold_rad_s_{0.10};
  std::string cmd_vel_topic_{"/cmd_vel_to_hw"};

  std::array<double, 4> target_wheel_rad_s_{};  // FL, FR, RL, RR
  std::array<std::unique_ptr<L298nMotor>, 4> motors_{};
  SteadyClock::time_point last_command_time_{};

  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_sub_;
  rclcpp::TimerBase::SharedPtr update_timer_;
};

}  // namespace robot_driver

#endif  // robot_driver__MECANUM_MOTOR_DRIVER_HPP_

