#include <gpio_output.hpp>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>

#include <gpiod.h>

namespace robot_driver
{
namespace
{
std::runtime_error makeGpioError(const std::string & operation, const std::string & label)
{
  const int error_number = errno;
  return std::runtime_error(
    operation + " for " + label + ": " + std::strerror(error_number));
}
}  // namespace

GpioOutput::GpioOutput(
  const std::string & chip_path,
  const std::uint32_t offset,
  const std::string & consumer)
: chip_path_(chip_path), offset_(offset)
{
  if (chip_path_.empty()) {
    throw std::invalid_argument("GPIO chip path cannot be empty for " + consumer);
  }

  chip_ = gpiod_chip_open(chip_path_.c_str());
  if (chip_ == nullptr) {
    throw makeGpioError("Unable to open " + chip_path_, consumer);
  }

#if defined(MECANUM_LIBGPIOD_V2)
  gpiod_line_settings * settings = gpiod_line_settings_new();
  if (settings == nullptr) {
    const auto error = makeGpioError("Unable to allocate GPIO line settings", consumer);
    release();
    throw error;
  }

  gpiod_line_config * line_config = gpiod_line_config_new();
  if (line_config == nullptr) {
    const auto error = makeGpioError("Unable to allocate GPIO line config", consumer);
    gpiod_line_settings_free(settings);
    release();
    throw error;
  }

  gpiod_request_config * request_config = gpiod_request_config_new();
  if (request_config == nullptr) {
    const auto error = makeGpioError("Unable to allocate GPIO request config", consumer);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);
    release();
    throw error;
  }

  gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
  gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
  gpiod_request_config_set_consumer(request_config, consumer.c_str());

  const unsigned int line_offset = offset_;
  if (gpiod_line_config_add_line_settings(line_config, &line_offset, 1, settings) < 0) {
    const auto error = makeGpioError("Unable to configure GPIO output", consumer);
    gpiod_request_config_free(request_config);
    gpiod_line_config_free(line_config);
    gpiod_line_settings_free(settings);
    release();
    throw error;
  }

  request_ = gpiod_chip_request_lines(chip_, request_config, line_config);
  const int request_errno = errno;

  gpiod_request_config_free(request_config);
  gpiod_line_config_free(line_config);
  gpiod_line_settings_free(settings);

  if (request_ == nullptr) {
    errno = request_errno;
    const auto error = makeGpioError("Unable to request GPIO output", consumer);
    release();
    throw error;
  }
#else
  line_ = gpiod_chip_get_line(chip_, offset_);
  if (line_ == nullptr) {
    const auto error = makeGpioError(
      "Unable to get GPIO offset " + std::to_string(offset_) + " from " + chip_path_, consumer);
    release();
    throw error;
  }

  if (gpiod_line_request_output(line_, consumer.c_str(), 0) < 0) {
    const auto error = makeGpioError("Unable to request GPIO output", consumer);
    release();
    throw error;
  }
  requested_ = true;
#endif

  cached_value_ = false;
}

GpioOutput::~GpioOutput() noexcept
{
  // Best-effort deassertion before releasing the output line.
  setNoThrow(false);
  release();
}

void GpioOutput::set(const bool high)
{
  if (!valid()) {
    throw std::runtime_error("Attempted to write an unrequested GPIO line");
  }
  if (high == cached_value_) {
    return;
  }

#if defined(MECANUM_LIBGPIOD_V2)
  const auto value = high ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
  if (gpiod_line_request_set_value(request_, offset_, value) < 0) {
    throw makeGpioError(
            "Unable to set " + chip_path_ + " offset " + std::to_string(offset_), "GPIO output");
  }
#else
  if (gpiod_line_set_value(line_, high ? 1 : 0) < 0) {
    throw makeGpioError(
            "Unable to set " + chip_path_ + " offset " + std::to_string(offset_), "GPIO output");
  }
#endif

  cached_value_ = high;
}

void GpioOutput::setNoThrow(const bool high) noexcept
{
  try {
    set(high);
  } catch (...) {
    // Destructors and emergency cleanup must not throw.
  }
}

bool GpioOutput::valid() const noexcept
{
#if defined(MECANUM_LIBGPIOD_V2)
  return chip_ != nullptr && request_ != nullptr;
#else
  return chip_ != nullptr && line_ != nullptr && requested_;
#endif
}

void GpioOutput::release() noexcept
{
#if defined(MECANUM_LIBGPIOD_V2)
  if (request_ != nullptr) {
    gpiod_line_request_release(request_);
    request_ = nullptr;
  }
#else
  if (line_ != nullptr && requested_) {
    gpiod_line_release(line_);
    requested_ = false;
  }
  line_ = nullptr;
#endif

  if (chip_ != nullptr) {
    gpiod_chip_close(chip_);
    chip_ = nullptr;
  }
}

}  // namespace robot_driver
