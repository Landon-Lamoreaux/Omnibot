#include <pointcloud_processing.hpp>

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PointcloudProcessingNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}