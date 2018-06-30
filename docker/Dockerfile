FROM ubuntu:rolling
LABEL maintainer "pschmied <ps1337@mailbox.org>"

# Dependencies
RUN apt-get update && \
    apt-get -y install \
        cmake \
        curl \
        g++ \
        gcc \
        git-core \
        gosu \
        libqt5svg5-dev \
        make \
        pkg-config \
        python3 \
        python3-dev \
        qtbase5-dev \
        qtwebengine5-dev \
        unzip \
        wget

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
    mv radare-radare2* radare2 && \
    cd radare2 && ./sys/install.sh

# Build cutter
RUN mkdir build
WORKDIR /opt/cutter/build
RUN cmake ../src && \
    make

# Add r2 user
RUN useradd r2

# Prepare files to mount configurations later on
RUN mkdir /var/sharedFolder && \
    mkdir -p /home/r2/.config/radare2 && \
    touch /home/r2/.radare2rc

RUN chown -R r2:r2 /var/sharedFolder && \
    chown -R r2:r2 /home/r2/

WORKDIR /home/r2
ADD entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
