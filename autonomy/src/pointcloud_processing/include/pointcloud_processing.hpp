#include "rclcpp/rclcpp.hpp"

#include "laser_geometry/laser_geometry.hpp"
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <std_msgs/msg/header.hpp>

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
// #include <pcl_conversions/pcl_conversions.h>

class PointcloudProcessingNode : public rclcpp::Node
{
    public:
        PointcloudProcessingNode();
        ~PointcloudProcessingNode();

    private:

        void points_callback(const sensor_msgs::msg::LaserScan::SharedPtr scan);
        
        rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr points_sub_;

        rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr filtered_pointcloud_pub_;

        laser_geometry::LaserProjection projector_;
};