#include <pointcloud_processing.hpp>

// Currently just republishing the lidar data with no modifications.
PointcloudProcessingNode::PointcloudProcessingNode() : Node("pointcloud_processing_node") {
  points_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>("/lidar/scan", 10, std::bind(&PointcloudProcessingNode::points_callback, this, std::placeholders::_1));
  filtered_pointcloud_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("filtered_pointcloud", 10);
}

PointcloudProcessingNode::~PointcloudProcessingNode() {
  return;
}


void PointcloudProcessingNode::points_callback(const sensor_msgs::msg::LaserScan::SharedPtr scan) {
  auto cloud = std::make_unique<sensor_msgs::msg::PointCloud2>();

  projector_.projectLaser(*scan, *cloud);

  filtered_pointcloud_pub_->publish(std::move(cloud));

}