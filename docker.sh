#!/bin/sh

docker build \
 --build-arg uid=$(id -u) --build-arg gid=$(id -g) \
 -t r2cutter \
 - <<\#\#\ DOCKERFILE\ END\ \#\# \
|| exit 1
######################
## DOCKERFILE START ##
######################

# A lighter debian distribution should be used
FROM ubuntu:xenial

# www.github.com/danielhenrymantilla
MAINTAINER daniel.henry.mantilla@gmail.com

# Dependencies
RUN apt-get update
RUN apt-get install -y build-essential
RUN apt-get install -y git nano wget
RUN apt-get install -y libdbus-1-3 libx11-xcb1
RUN apt-get install -y libdrm-common libdrm-dev libgl1-mesa-dev pkg-config libsm6
RUN apt-get install -y libfontconfig1-dev libfreetype6-dev libx11-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libx11-xcb-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev
RUN rm -rf /var/cache/apt/
ENV QT_XKB_CONFIG_ROOT=/usr/share/X11/xkb

# Using benlau's qtci tool to headless-ly install QT 5.9.1
RUN git clone https://github.com/benlau/qtci /tmp/qtci
ENV PATH="/tmp/qtci/bin:/tmp/qtci/recipes:$PATH"

# Download the QT 5.9.1 installer (1G) and check its md5sum
WORKDIR /tmp/qtci/recipes/
RUN echo "Downloading Qt..." && wget https://download.qt.io/archive/qt/5.9/5.9.1/qt-opensource-linux-x64-5.9.1.run && echo "Downloaded. Checking md5sum..." && ([ "b8dd904894ac6b09076b5f61d8b9d71a" = "$(md5sum qt-opensource-linux-x64-5.9.1.run | cut -d\  -f1)" ] || (echo "Error: MD5 checksum mismatch"; false)) && echo "QT installer downloaded successfully"

# Install QT 5.9.1
ENV QT_CI_PACKAGES=qt.591.gcc_64
RUN sync && extract-qt-installer "qt-opensource-linux-x64-5.9.1.run" /opt/Qt
RUN rm "qt-opensource-linux-x64-5.9.1.run"
RUN find /opt/Qt/5.9.1/gcc_64/bin -type f -executable -exec ln -s {} /usr/bin \;

# Download r2 and cutter
RUN git clone --recurse-submodules https://github.com/radareorg/cutter /tmp/cutter
WORKDIR /tmp/cutter

# Compilation & Installation
RUN sed -i -e 's/#QMAKE_CONF/QMAKE_CONF/' build.sh
RUN printf "\n" | ./build.sh
RUN make -C build clean
RUN ln -s /tmp/cutter/build/Cutter /usr/bin/r2cutter

#### Creating guest's user ####
# Use '--build-arg uid=$(id -u)' and '--build-arg gid=$(id -g)' at build time
# to match the guest's uid and gid with the host's
ARG uid=1000
ARG gid=1000
RUN echo "Got uid = ${uid} and gid = ${gid}"
RUN mkdir -p /home/reverser && \
    mkdir -p /etc/sudoers.d && \
    echo "reverser:x:${uid}:${gid}:Reverser,,,:/home/reverser:/bin/bash" >> /etc/passwd && \
    echo "reverser:x:${uid}:" >> /etc/group && \
    echo "reverser ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/reverser && \
    chmod 0440 /etc/sudoers.d/reverser && \
    chown ${uid}:${gid} -R /home/reverser
USER reverser
ENV HOME="/home/reverser"

# Entrypoint
WORKDIR /home/reverser
CMD r2cutter 2>/dev/null

####################
## DOCKERFILE END ##
####################

############
## X auth ##
XSOCK=/tmp/.X11-unix
XAUTH=$(mktemp /tmp/r2cutter_tmp.XXX.xauth)
touch $XAUTH
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -
############

####################
## RUN DOCKER CMD ##
if [ -z "$1" ]; then
	docker run -ti --rm \
	 -e DISPLAY \
	 -u reverser \
	 -v $XSOCK:$XSOCK:rw \
	 -v $XAUTH:/home/reverser/.Xauthority:rw \
	 -v ~/.r2cutter_docker_config:/home/reverser/.config \
	 r2cutter
else # Arg $1 may be mounted (read-only) to the docker
	BINARY="$(echo "$1" | sed -e "s/^\.\//$(pwd | sed -e 's/\//\\\//g')\//")"
	R2BINARY="$(basename "$1")"
	docker run -ti --rm \
	 -e DISPLAY \
	 -u reverser \
	 -v $XSOCK:$XSOCK:rw \
	 -v $XAUTH:/home/reverser/.Xauthority:rw \
	 -v ~/.r2cutter_docker_config:/home/reverser/.config \
	 "-v${BINARY}:/home/reverser/${R2BINARY}:ro" \
	 r2cutter \
	 sh -c "r2cutter \"${R2BINARY}\" 2>/dev/null"
fi
####################

##############
## Cleanup ###
rm -f $XAUTH #
##############
