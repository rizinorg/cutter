# Docker Configuration for Cutter

These files provide an easy way to deploy *Cutter* in a Docker container. After additional configuration you may want to apply to the `Makefile`, execute `make run`. By default, the *Cutter* image on [Docker Hub](https://hub.docker.com/r/radareorg/cutter/) will be used along with additional UID, capability, X and mount settings:

- Xauthority settings which avoid using potentially insecure `xhost` directives. The settings have been adapted from [this post](https://stackoverflow.com/questions/16296753/can-you-run-gui-apps-in-a-docker-container/25280523#25280523).
- Mount directives to mount a shared folder and radare2 configuration files.
- The UID and GID of the user executing `make run` will also be used for the internal container user to avoid permission problems when sharing files.

## Mounting and Using a Specific Binary

The `Makefile` allows mounting a single binary file as read-only, which will also be used as an input for *Cutter*. To use this feature, execute `make run BINARY=/absolute/path/to/binary`.

## Additional Notes

- The internal container user doesn't use superuser privileges and is called `r2`.
- To check for more options of the `Makefile`, execute `make`.
