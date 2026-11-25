ARG ARCHI_IMAGE=ros:lyrical-ros-base

FROM $ARCHI_IMAGE AS nav2
SHELL ["/bin/bash", "-c"]

# Installing "NAV2".
RUN set -eux; \
    apt-get update; \ 
    # apt-get -y upgrade; \
    apt-get install -y --no-install-recommends ros-lyrical-navigation2 ros-lyrical-nav2-bringup \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/* /tmp/* /var/tmp/*


FROM $ARCHI_IMAGE AS common

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3-pip \
        python3-venv \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/* /tmp/* /var/tmp/*

# Installing Python dependencies in a virtualenv.
WORKDIR /usr/src/app
COPY ./autonomy/requirements.txt ./
RUN python3 -m venv /venv && \
    . /venv/bin/activate && \
    pip install --no-cache-dir -r requirements.txt

# Purging pip cache, __pycache__, and other unnecessary files.
RUN . /venv/bin/activate && \
    pip cache purge && \
    find /venv -type d -name '__pycache__' -exec rm -r {} + && \
    find /venv -type d -name 'tests' -exec rm -r {} + && \
    rm -rf /venv/share/man /venv/share/doc /venv/share/locale /venv/include

# Building the custom messages package.
FROM nav2 AS msgs
SHELL ["/bin/bash", "-c"]
COPY ./autonomy/src/msgs /usr/src/msgs
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    cd /usr/src/msgs; \
    colcon build

# Building the pointcloud processing node.
FROM nav2 AS pointcloud_processing
SHELL ["/bin/bash", "-c"]
RUN apt-get update && \
    apt-get -y install \
        ros-lyrical-pcl-ros \
        ros-lyrical-pcl-conversions \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/* /tmp/* /var/tmp/*
COPY ./autonomy/src/pointcloud_processing /usr/src/pointcloud_processing
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    cd /usr/src/pointcloud_processing; \
    colcon build

# Building the launch files package.
FROM nav2 AS launch_files
SHELL ["/bin/bash", "-c"]
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
      ros-lyrical-xacro \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/* /tmp/* /var/tmp/*
COPY ./autonomy/src/launch_files /usr/src/launch_files/launch_files
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    cd /usr/src/launch_files; \
    colcon build

# Installing and setting up the latest version of Rviz.
FROM nav2 AS rviz
SHELL ["/bin/bash", "-c"]
RUN apt-get update && \
    apt-get install --no-install-recommends -y \
      qtbase5-dev \
      ros-lyrical-rviz2 \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/* /tmp/* /var/tmp/*
COPY ./autonomy/src/rviz /usr/src/rviz
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    cd /usr/src/rviz; \
    colcon build

# Building the behavior trees package.
FROM nav2 AS behavior_trees
SHELL ["/bin/bash", "-c"]
COPY ./autonomy/src/behavior_trees /usr/src/behavior_trees
COPY --from=msgs /usr/src/msgs/install /usr/src/msgs/install
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    . /usr/src/msgs/install/setup.bash; \
    cd /usr/src/behavior_trees; \
    colcon build

# Building the driver node that converts cmd_vel to motor commands.
FROM nav2 AS robot_driver
SHELL ["/bin/bash", "-c"]
COPY ./autonomy/src/robot_driver /usr/src/robot_driver
COPY --from=msgs /usr/src/msgs/install /usr/src/msgs/install
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    . /usr/src/msgs/install/setup.bash; \
    cd /usr/src/robot_driver; \
    colcon build

# Building the gamepad node.
FROM nav2 AS gamepad
SHELL ["/bin/bash", "-c"]
COPY ./autonomy/src/gamepad /usr/src/gamepad
COPY --from=msgs /usr/src/msgs/install /usr/src/msgs/install
RUN apt-get update && apt-get install -y \
    ros-lyrical-joy \
    && rm -rf /var/lib/apt/lists/*
RUN set -ex; \
    . /opt/ros/lyrical/setup.bash; \
    . /usr/src/msgs/install/setup.bash; \
    cd /usr/src/gamepad; \
    colcon build

FROM nav2 AS run
SHELL ["/bin/bash", "-c"]

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      ros-lyrical-ros-gz \
      xvfb \
      x11vnc \
      ros-lyrical-rviz2 \
      ros-lyrical-rqt-reconfigure \
      ros-lyrical-rqt-common-plugins \
      ros-lyrical-pcl-ros \
      ros-lyrical-openvdb-vendor \
      ros-lyrical-twist-mux \
      ros-lyrical-joint-state-publisher \
      ros-lyrical-spatio-temporal-voxel-layer \
      ros-lyrical-joy \
      vim \
      valgrind \
      heaptrack \
      linux-tools-common \
      linux-tools-generic \
      rsync \
    && rm -rf /var/lib/apt/lists/* /var/cache/apt/* /var/log/apt/* /var/log/* /tmp/* /var/tmp/*

RUN touch /root/.bashrc && \
    echo ". /opt/ros/lyrical/setup.bash" >> /root/.bashrc; \
    echo ". /usr/src/pointcloud_processing/install/setup.bash" >> /root/.bashrc ;\
    echo ". /usr/src/behavior_trees/install/setup.bash" >> /root/.bashrc; \
    echo ". /usr/src/robot_driver/install/setup.bash" >> /root/.bashrc; \
    echo ". /usr/src/gamepad/install/setup.bash" >> /root/.bashrc; \
    echo ". /usr/src/launch_files/install/setup.bash" >> /root/.bashrc; \
    echo ". /usr/src/msgs/install/setup.bash" >> /root/.bashrc;

COPY --from=pointcloud_processing /usr/src/pointcloud_processing/install /usr/src/pointcloud_processing/install
COPY --from=launch_files /usr/src/launch_files/install /usr/src/launch_files/install
COPY --from=behavior_trees /usr/src/behavior_trees /usr/src/behavior_trees
COPY --from=robot_driver /usr/src/robot_driver /usr/src/robot_driver
COPY --from=gamepad /usr/src/gamepad /usr/src/gamepad
COPY --from=msgs /usr/src/msgs/install /usr/src/msgs/install

COPY ./autonomy/entrypoint.sh /usr/src/entrypoint.sh

COPY --from=common /venv /venv

RUN mkdir -p /usr/src/valgrind

ENV PYTHONPATH=/venv/lib/python3.12/site-packages:$PYTHONPATH
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

RUN chmod +x /usr/src/entrypoint.sh

ENTRYPOINT ["/usr/src/entrypoint.sh"]
