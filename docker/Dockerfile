FROM alpine:edge AS builder
LABEL maintainer "Philipp Schmied <ps1337@mailbox.org>"

# Prevent build fails because of interactive scripts when compiling
ENV DEBIAN_FRONTEND noninteractive

# Dependencies
RUN apk add --no-cache \
        bash \
        cmake \
        curl \
        g++ \
        gcc \
        git \
        linux-headers \
        make \
        pkgconfig \
        python3-dev \
        qt5-qtbase \
        qt5-qtsvg-dev \
        qt5-qttools-dev \
        unzip \
        wget

# Get, compile and test Cutter from master branch
RUN git clone --recurse-submodules https://github.com/radareorg/cutter.git /opt/cutter
RUN cd /opt/cutter && \
    bash build.sh && \
    bash -c 'if [[ ! -x "/opt/cutter/build/Cutter" ]]; then exit -1; fi'

FROM alpine:edge AS runner

# Add the dependencies we need for running
RUN apk add --no-cache \
         bash \
         libuuid \
         make \
         python3 \
         qt5-qtbase \
         shadow \
         su-exec

# Get the compiled Cutter, r2 libs and bins from the builder
COPY --from=builder /opt/cutter /opt/cutter
COPY --from=builder /usr/lib /usr/lib
COPY --from=builder /usr/share/radare2 /usr/share/radare2
RUN cd /opt/cutter/radare2/binr && \
    make install && \
    make symstall install-symlink

# Add r2 user
RUN useradd r2

# Prepare files to mount configurations later on
RUN mkdir /var/sharedFolder && \
    mkdir -p /home/r2/.config/radare2 && \
    touch /home/r2/.radare2rc && \
    chown -R r2:r2 /var/sharedFolder && \
    chown -R r2:r2 /home/r2/

WORKDIR /home/r2
ADD entrypoint.sh /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
