#ifndef ROBOT_DRIVER__GPIO_OUTPUT_HPP_
#define ROBOT_DRIVER__GPIO_OUTPUT_HPP_

#include <cstdint>
#include <string>

struct gpiod_chip;
struct gpiod_line;
struct gpiod_line_request;

namespace robot_driver
{

/**
 * RAII wrapper for one libgpiod output line.
 *
 * chip_path must be a GPIO character-device path (for example /dev/gpiochip0).
 * offset is the line offset within that chip; it is NOT the 40-pin header pin
 * number and is NOT a legacy global GPIO number.
 *
 * The implementation uses the libgpiod C API from C++, allowing it to build
 * against both libgpiod 1.x and 2.x. The API generation is selected by CMake.
 */
class GpioOutput
{
public:
  GpioOutput(
    const std::string & chip_path,
    std::uint32_t offset,
    const std::string & consumer);
  ~GpioOutput() noexcept;

  GpioOutput(const GpioOutput &) = delete;
  GpioOutput & operator=(const GpioOutput &) = delete;
  GpioOutput(GpioOutput &&) = delete;
  GpioOutput & operator=(GpioOutput &&) = delete;

  void set(bool high);
  void setNoThrow(bool high) noexcept;
  bool valid() const noexcept;

private:
  void release() noexcept;

  std::string chip_path_;
  std::uint32_t offset_{0};
  gpiod_chip * chip_{nullptr};

#if defined(MECANUM_LIBGPIOD_V2)
  gpiod_line_request * request_{nullptr};
#else
  gpiod_line * line_{nullptr};
  bool requested_{false};
#endif

  bool cached_value_{false};
};

}  // namespace robot_driver

#endif  // ROBOT_DRIVER__GPIO_OUTPUT_HPP_
