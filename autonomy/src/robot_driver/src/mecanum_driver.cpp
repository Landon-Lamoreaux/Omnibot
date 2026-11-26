#include <mecanum_driver.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace robot_driver
{

RobotDriver::RobotDriver(const rclcpp::NodeOptions & options)
: Node("robot_driver", options)
{
  loadParametersAndInitializeHardware();

  const auto qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable();
  cmd_sub_ = create_subscription<geometry_msgs::msg::TwistStamped>(
    cmd_vel_topic_, qos,
    std::bind(&RobotDriver::commandCallback, this, std::placeholders::_1));

  update_timer_ = create_wall_timer(
    std::chrono::duration<double>(1.0 / update_rate_hz_),
    std::bind(&RobotDriver::updateHardware, this));

  RCLCPP_INFO(
    get_logger(), "Mecanum motor driver listening for TwistStamped on '%s'",
    cmd_vel_topic_.c_str());

  if (!hardware_enabled_) {
    RCLCPP_WARN(
      get_logger(),
      "hardware_enabled=false: dry-run mode. GPIO lines are not requested and motors will not move.");
  } else {
    RCLCPP_WARN(
      get_logger(),
      "GPIO-only L298N mode: enabled motors run at full supply voltage; no PWM speed control is active. "
      "Commission with wheels raised and an independent hardware E-stop.");
  }
}

RobotDriver::~RobotDriver()
{
  stopAll();
}

void RobotDriver::validateFourEntries(
  const std::vector<std::string> & values, const std::string & name) const
{
  if (values.size() != 4) {
    throw std::invalid_argument(name + " must have exactly four entries (FL, FR, RL, RR)");
  }
}

void RobotDriver::validateFourEntries(
  const std::vector<std::int64_t> & values, const std::string & name) const
{
  if (values.size() != 4) {
    throw std::invalid_argument(name + " must have exactly four entries (FL, FR, RL, RR)");
  }
}

void RobotDriver::loadParametersAndInitializeHardware()
{
  hardware_enabled_ = declare_parameter<bool>("hardware_enabled", false);
  cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "/cmd_vel_to_hw");

  wheel_radius_ = declare_parameter<double>("wheel_radius", 0.12);
  half_wheelbase_ = declare_parameter<double>("half_wheelbase", 0.35);
  half_track_ = declare_parameter<double>("half_track", 0.27);
  command_timeout_sec_ = declare_parameter<double>("command_timeout_sec", 0.30);
  update_rate_hz_ = declare_parameter<double>("update_rate_hz", 50.0);
  enable_threshold_rad_s_ = declare_parameter<double>("enable_threshold_rad_s", 0.10);

  if (cmd_vel_topic_.empty() || wheel_radius_ <= 0.0 || half_wheelbase_ < 0.0 ||
    half_track_ < 0.0 || command_timeout_sec_ <= 0.0 || update_rate_hz_ <= 0.0 ||
    enable_threshold_rad_s_ < 0.0)
  {
    throw std::invalid_argument("Invalid cmd_vel_topic, geometry, timeout, rate, or enable threshold");
  }

  const auto motor_polarity = declare_parameter<std::vector<std::int64_t>>(
    "motor_polarity", {1, 1, 1, 1});
  validateFourEntries(motor_polarity, "motor_polarity");
  for (const auto polarity : motor_polarity) {
    if (polarity != 1 && polarity != -1) {
      throw std::invalid_argument("motor_polarity entries must each be +1 or -1");
    }
  }

  const auto in1_chips = declare_parameter<std::vector<std::string>>("in1_chip_paths", {});
  const auto in1_offsets = declare_parameter<std::vector<std::int64_t>>("in1_offsets", {});
  const auto in2_chips = declare_parameter<std::vector<std::string>>("in2_chip_paths", {});
  const auto in2_offsets = declare_parameter<std::vector<std::int64_t>>("in2_offsets", {});
  const auto enable_chips = declare_parameter<std::vector<std::string>>("enable_chip_paths", {});
  const auto enable_offsets = declare_parameter<std::vector<std::int64_t>>("enable_offsets", {});

  if (!hardware_enabled_) {
    return;
  }

  validateFourEntries(in1_chips, "in1_chip_paths");
  validateFourEntries(in1_offsets, "in1_offsets");
  validateFourEntries(in2_chips, "in2_chip_paths");
  validateFourEntries(in2_offsets, "in2_offsets");
  validateFourEntries(enable_chips, "enable_chip_paths");
  validateFourEntries(enable_offsets, "enable_offsets");

  const std::array<std::string, 4> names{
    "front_left", "front_right", "rear_left", "rear_right"};

  // Catch accidentally assigning one physical GPIO line to multiple signals.
  std::set<std::string> used_lines;
  const auto check_line = [&used_lines](const std::string & chip, const std::int64_t offset,
      const std::string & signal)
    {
      if (chip.empty() || offset < 0 ||
        static_cast<std::uint64_t>(offset) > std::numeric_limits<std::uint32_t>::max())
      {
        throw std::invalid_argument(signal + " has an invalid chip path or line offset");
      }
      const std::string key = chip + ":" + std::to_string(offset);
      if (!used_lines.insert(key).second) {
        throw std::invalid_argument("GPIO line " + key + " is assigned to more than one signal");
      }
    };

  for (std::size_t i = 0; i < 4; ++i) {
    check_line(in1_chips[i], in1_offsets[i], names[i] + " IN1");
    check_line(in2_chips[i], in2_offsets[i], names[i] + " IN2");
    check_line(enable_chips[i], enable_offsets[i], names[i] + " EN");
  }

  // Construction requests each EN output LOW before its direction pins.
  // If any request fails, already-created motor objects destruct and release
  // their requested lines; their EN outputs are left LOW on best effort.
  for (std::size_t i = 0; i < 4; ++i) {
    motors_[i] = std::make_unique<L298nMotor>(
      in1_chips[i], static_cast<std::uint32_t>(in1_offsets[i]),
      in2_chips[i], static_cast<std::uint32_t>(in2_offsets[i]),
      enable_chips[i], static_cast<std::uint32_t>(enable_offsets[i]),
      static_cast<int>(motor_polarity[i]), names[i]);
  }

  RCLCPP_INFO(get_logger(), "Requested all 12 L298N GPIO output lines.");
}

void RobotDriver::commandCallback(
  const geometry_msgs::msg::TwistStamped::ConstSharedPtr msg)
{
  const double vx = msg->twist.linear.x;
  const double vy = msg->twist.linear.y;
  const double wz = msg->twist.angular.z;

  if (!std::isfinite(vx) || !std::isfinite(vy) || !std::isfinite(wz)) {
    RCLCPP_ERROR_THROTTLE(
      get_logger(), *get_clock(), 2000, "Rejecting non-finite TwistStamped command; stopping motors.");
    target_wheel_rad_s_.fill(0.0);
    command_received_ = false;
    return;
  }

  // Mecanum inverse kinematics. Wheel order: FL, FR, RL, RR.
  // The physical motor_polarity parameter compensates for wiring orientation.
  const double k = half_wheelbase_ + half_track_;
  target_wheel_rad_s_ = {
    (vx - vy - k * wz) / wheel_radius_,
    (vx + vy + k * wz) / wheel_radius_,
    (vx + vy - k * wz) / wheel_radius_,
    (vx - vy + k * wz) / wheel_radius_
  };

  last_command_time_ = SteadyClock::now();
  command_received_ = true;
  timed_out_ = false;
}

void RobotDriver::updateHardware()
{
  if (!hardware_enabled_ || hardware_faulted_) {
    return;
  }

  const bool command_is_fresh = command_received_ &&
    (SteadyClock::now() - last_command_time_) <
    std::chrono::duration<double>(command_timeout_sec_);

  if (!command_is_fresh) {
    if (!timed_out_) {
      RCLCPP_WARN(get_logger(), "No fresh velocity command; disabling all L298N channels.");
      timed_out_ = true;
    }
    stopAll();
    return;
  }

  try {
    for (std::size_t i = 0; i < motors_.size(); ++i) {
      if (motors_[i]) {
        motors_[i]->apply(target_wheel_rad_s_[i], enable_threshold_rad_s_);
      }
    }
  } catch (const std::exception & error) {
    hardware_faulted_ = true;
    command_received_ = false;
    RCLCPP_FATAL(
      get_logger(), "GPIO hardware error: %s. Disabling all motor channels; restart node after resolving fault.",
      error.what());
    stopAll();
  }
}

void RobotDriver::stopAll() noexcept
{
  for (auto & motor : motors_) {
    if (motor) {
      try {
        motor->stop();
      } catch (...) {
        // Best effort. External EN pull-downs and a hardware E-stop are still required.
      }
    }
  }
}

}  // namespace robot_driver
