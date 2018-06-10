FROM ubuntu:rolling
LABEL maintainer "pschmied <ps1337@mailbox.org>"

# Dependencies
RUN apt-get update && \
    apt-get -y install \
    curl \
    libqt5svg5-dev \
    make \
    qtbase5-dev \
    qtwebengine5-dev \
    unzip \
    wget \
    cmake \
    g++ \
    gcc \
    git-core \
    python3 \
    python3-dev \
    pkg-config

# Get latest cutter release
WORKDIR /opt
RUN curl https://api.github.com/repos/radareorg/cutter/releases/latest | \
        grep "zipball_url" | \
        tr -d ",\" " | \
        cut -d ":" -f 2,3 | \
    wget -O cutter.zip -i - && \
    unzip cutter.zip && \
    rm cutter.zip && \
    mv radareorg-cutter* cutter

# Get latest radare2 release and build it
WORKDIR /opt/cutter
RUN rm -rf radare2 && \
    curl https://api.github.com/repos/radare/radare2/releases/latest | \
        grep "zipball_url" | \
        tr -d ",\" " | \
        cut -d ":" -f 2,3 | \
    wget -O radare2.zip -i - && \
    unzip radare2.zip && \
    rm radare2.zip && \
    mv radare-radare2* radare2

RUN cd radare2 && ./sys/install.sh

# Build cutter
RUN mkdir build
WORKDIR /opt/cutter/build
RUN cmake ../src
RUN make

# Add r2 user
RUN useradd r2

WORKDIR /home/r2
RUN mkdir /var/sharedFolder
RUN mkdir -p /home/r2/.config/radare2
RUN touch /home/r2/.radare2rc

RUN chown -R r2:r2 /var/sharedFolder
RUN chown -R r2:r2 /home/r2/
USER r2

ENTRYPOINT ["/bin/bash", "-c", "/opt/cutter/build/cutter"]
