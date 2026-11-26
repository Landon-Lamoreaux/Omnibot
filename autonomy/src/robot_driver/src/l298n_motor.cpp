#include <l298n_motor.hpp>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace robot_driver
{

L298nMotor::L298nMotor(
  const std::string & in1_chip, const std::uint32_t in1_offset,
  const std::string & in2_chip, const std::uint32_t in2_offset,
  const std::string & enable_chip, const std::uint32_t enable_offset,
  const int polarity, const std::string & motor_name)
: enable_(enable_chip, enable_offset, motor_name + "_enable"),
  in1_(in1_chip, in1_offset, motor_name + "_in1"),
  in2_(in2_chip, in2_offset, motor_name + "_in2"),
  polarity_(polarity),
  motor_name_(motor_name)
{
  if (polarity_ != 1 && polarity_ != -1) {
    throw std::invalid_argument(
      "Motor polarity for " + motor_name_ + " must be either +1 or -1");
  }
  if (motor_name_.empty()) {
    throw std::invalid_argument("Motor name cannot be empty");
  }

  // Keep the L298N channel disabled while setting the initial direction pins.
  // The GpioOutput constructor requests each line with an initial LOW value.
  enable_.set(false);
  in1_.set(false);
  in2_.set(false);
  last_direction_ = 0;
}

void L298nMotor::apply(
  const double wheel_rad_s, const double enable_threshold_rad_s)
{
  if (!std::isfinite(wheel_rad_s)) {
    throw std::invalid_argument("Non-finite wheel command for " + motor_name_);
  }
  if (!std::isfinite(enable_threshold_rad_s) || enable_threshold_rad_s < 0.0) {
    throw std::invalid_argument(
      "Enable threshold for " + motor_name_ + " must be finite and non-negative");
  }

  // GPIO-only mode cannot scale speed. The wheel-speed magnitude is used only
  // to decide whether the channel should be enabled; its sign sets direction.
  const double polarized_speed = wheel_rad_s * static_cast<double>(polarity_);
  if (std::abs(polarized_speed) <= enable_threshold_rad_s) {
    stop();
    return;
  }

  const int desired_direction = (polarized_speed > 0.0) ? 1 : -1;

  if (desired_direction != last_direction_) {
    // Disable the bridge before changing direction inputs to avoid changing
    // direction while the output stage is enabled.
    enable_.set(false);

    if (desired_direction > 0) {
      in1_.set(true);
      in2_.set(false);
    } else {
      in1_.set(false);
      in2_.set(true);
    }

    last_direction_ = desired_direction;
  }

  // Enable at full available motor-supply voltage. No PWM speed control here.
  enable_.set(true);
}

void L298nMotor::stop()
{
  // Disable first, then return the direction inputs to a known state.
  enable_.set(false);
  in1_.set(false);
  in2_.set(false);
  last_direction_ = 0;
}

}  // namespace mecanum_hardware
