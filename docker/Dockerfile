FROM alpine:latest AS builder
LABEL maintainer "Philipp Schmied <ps1337@mailbox.org>"

# Prevent build fails because of interactive scripts when compiling
ENV DEBIAN_FRONTEND noninteractive

# Dependencies
RUN apk add --no-cache \
        bash \
        build-base \
        cmake \
        curl \
        git \
        linux-headers \
        pkgconfig \
        python3-dev \
        qt5-qtbase \
        qt5-qtsvg-dev \
        qt5-qttools-dev \
        unzip \
        wget

# Get, compile and test Cutter from master branch
RUN git clone --recurse-submodules https://github.com/rizinorg/cutter.git /opt/cutter
RUN cd /opt/cutter && \
    bash build.sh && \
    bash -c 'if [[ ! -x "/opt/cutter/build/Cutter" ]]; then exit -1; fi'

FROM alpine:latest AS runner

# Add the dependencies we need for running
RUN apk add --no-cache \
         bash \
         libuuid \
         make \
         python3 \
         qt5-qtbase \
         shadow \
         su-exec

# Get the compiled Cutter, rizin libs and bins from the builder
COPY --from=builder /opt/cutter /opt/cutter
COPY --from=builder /usr/lib /usr/lib
COPY --from=builder /usr/share/rizin /usr/share/rizin
RUN cd /opt/cutter/rizin/binr && \
    make install && \
    make symstall install-symlink

# Add rizin user
RUN useradd rizin

# Prepare files to mount configurations later on
RUN mkdir /var/sharedFolder && \
    mkdir -p /home/rizin/.config/rizin && \
    touch /home/rizin/.rizinrc && \
    chown -R rizin:rizin /var/sharedFolder && \
    chown -R rizin:rizin /home/rizin/

WORKDIR /home/rizin
ADD entrypoint.sh /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
