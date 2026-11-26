#include <mecanum_driver.hpp>

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<robot_driver::RobotDriver>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}