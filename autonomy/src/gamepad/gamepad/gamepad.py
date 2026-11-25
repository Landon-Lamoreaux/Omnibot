
#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
import time

from sensor_msgs.msg import Joy
from geometry_msgs.msg import TwistStamped


class Gamepad(Node):

    def __init__(self):
        super().__init__('gamepad')

        # Controller mapping parameter declaration.
        self.declare_parameter('toggle_button', 6)  # OPTIONS
        self.declare_parameter('forward_axis', 1)
        self.declare_parameter('strafe_axis', 0)
        self.declare_parameter('yaw_axis', 2)

        # Maximum velocities parameter declaration.
        self.declare_parameter('forward_scale', 0.35)
        self.declare_parameter('strafe_scale', 0.30)
        self.declare_parameter('yaw_scale', 0.70)

        # Input settings parameter declaration.
        self.declare_parameter('deadzone', 0.08)
        self.declare_parameter('joy_timeout', 0.35)
        self.declare_parameter('publish_rate', 30.0)

        self.toggle_armed = False

        # Getting the parameter values.
        self.toggle_button = int(self.get_parameter('toggle_button').value)
        self.forward_axis = int(self.get_parameter('forward_axis').value)
        self.strafe_axis = int(self.get_parameter('strafe_axis').value)
        self.yaw_axis = int(self.get_parameter('yaw_axis').value)
        self.forward_scale = float(self.get_parameter('forward_scale').value)
        self.strafe_scale = float(self.get_parameter('strafe_scale').value)
        self.yaw_scale = float(self.get_parameter('yaw_scale').value)
        self.deadzone = float(self.get_parameter('deadzone').value)
        self.joy_timeout = float(self.get_parameter('joy_timeout').value)
        publish_rate = float(self.get_parameter('publish_rate').value)

        if publish_rate <= 0.0:
            raise ValueError('publish_rate must be positive')

        self.axes = []
        self.buttons = []

        self.last_joy_time = None

        # Setting gamepad use to start disabled.
        self.enabled = False

        # To detect button press transitions.
        self.previous_toggle_pressed = False

        # Setting up subscribers, publishers, and timers.
        self.cmd_pub = self.create_publisher(TwistStamped, "/cmd_vel_joy", 10)
        self.joy_sub = self.create_subscription(Joy, "/joy", self.joy_callback, 10)
        self.timer = self.create_timer(1.0 / publish_rate, self.publish_command)

        self.get_logger().info("Gamepad teleop ready. Press the OPTIONS button to enable control.")

    def get_axis(self, index):
        if index < 0 or index >= len(self.axes):
            return 0.0

        value = max(-1.0, min(1.0, float(self.axes[index])))

        if abs(value) < self.deadzone:
            return 0.0

        return value

    def get_button(self, index):
        if index < 0 or index >= len(self.buttons):
            return False

        return bool(self.buttons[index])

    def publish_stop(self):
        self.cmd_pub.publish(TwistStamped())

    def joy_callback(self, msg):

        self.axes = list(msg.axes)
        self.buttons = list(msg.buttons)

        self.last_joy_time = time.monotonic()

        # Detect the OPTIONS buttons rising edge.
        toggle_pressed = self.get_button(self.toggle_button)

        # Require a released button before accepting a new press.
        if not toggle_pressed:
            self.toggle_armed = True
        elif self.toggle_armed and not self.previous_toggle_pressed:
            self.enabled = not self.enabled
            self.toggle_armed = False
            if self.enabled:
                self.get_logger().info('GAMEPAD ENABLED')
            else:
                self.get_logger().info('GAMEPAD DISABLED - STOPPING ROBOT')
                self.publish_stop()

        self.previous_toggle_pressed = toggle_pressed

    def publish_command(self):

        # No controller data received yet.
        if self.last_joy_time is None:
            return

        # Detect controller disconnect or lost messages.
        input_age = time.monotonic() - self.last_joy_time

        # Lost connection to controller, clean up and reset everything, disable controller.
        if input_age > self.joy_timeout:
            if self.enabled:
                self.enabled = False
                self.get_logger().warning("Controller disconnected or timed out. Stopping robot.")
                self.publish_stop()

            self.axes = []
            self.buttons = []
            self.previous_toggle_pressed = False
            self.toggle_armed = False
            self.last_joy_time = None

            return

        # Do not publish commands when manual mode is disabled.
        if not self.enabled:
            return

        cmd = TwistStamped()

        cmd.header.stamp = self.get_clock().now().to_msg()

        # Left stick: forward / reverse.
        cmd.twist.linear.x = (self.get_axis(self.forward_axis) * self.forward_scale)

        # Left stick: strafe.
        cmd.twist.linear.y = (self.get_axis(self.strafe_axis) * self.strafe_scale)

        # Right stick: rotation.
        cmd.twist.angular.z = (self.get_axis(self.yaw_axis) * self.yaw_scale)

        self.cmd_pub.publish(cmd)


def main(args=None):

    rclpy.init(args=args)

    node = Gamepad()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        pass

    finally:
        node.publish_stop()  # Sending a final stop command before shutting down.

        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
