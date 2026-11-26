#ifndef ROBOT_DRIVER__L298N_MOTOR_HPP_
#define ROBOT_DRIVER__L298N_MOTOR_HPP_

#include <cstdint>
#include <string>

#include "gpio_output.hpp"

namespace robot_driver
{

/**
 * One L298N full-bridge channel.
 *
 * IN1/IN2 select motor direction. ENA/ENB is used as a digital enable only:
 * LOW disables/coasts the channel; HIGH enables it at full motor supply.
 * This class does not generate PWM and therefore cannot scale motor speed.
 */
class L298nMotor
{
public:
  L298nMotor(
    const std::string & in1_chip, std::uint32_t in1_offset,
    const std::string & in2_chip, std::uint32_t in2_offset,
    const std::string & enable_chip, std::uint32_t enable_offset,
    int polarity, const std::string & motor_name);

  L298nMotor(const L298nMotor &) = delete;
  L298nMotor & operator=(const L298nMotor &) = delete;

  /** Apply a signed wheel-speed request. Only the sign is physically used. */
  void apply(double wheel_rad_s, double enable_threshold_rad_s);

  /** Disable the channel and put both direction inputs LOW. */
  void stop();

private:
  // Request EN first so the channel starts disabled before direction setup.
  GpioOutput enable_;
  GpioOutput in1_;
  GpioOutput in2_;
  int polarity_{1};
  std::string motor_name_;
  int last_direction_{0};
};

}  // namespace robot_driver

#endif  // ROBOT_DRIVER__L298N_MOTOR_HPP_
