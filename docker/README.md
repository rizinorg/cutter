# Docker Configuration for Cutter

These files provide an easy way to deploy *cutter* in a docker container.

You can use the pre-built image like:
```
touch $PWD/radare2rc && \
mkdir -p $PWD/r2-config && \
mkdir -p $PWD/sharedFolder && \
xhost +local:root && \
sudo docker run \
    -it \
    --name cutter \
    --cap-drop=ALL  \
    --cap-add=SYS_PTRACE \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    -v $PWD/sharedFolder:/var/sharedFolder \
    -v $PWD/radare2rc:/home/r2/.radare2rc \
    -v $PWD/r2-config:/home/r2/.config/radare2 \
    radare/cutter:latest
```

or by using the `Makefile` (after additional configuration to make it fit your needs) by executing `make build` and `make run`.
