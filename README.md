# clover

LPL firmware.

## Development Environment

we use devcontainers for dependency management and consistency across all developer machines.

Because microcontroller flashing must be run from the host OS, a daemon (`flasherd`) is started,
which provides a network endpoint mounted into the host from which flashing can yada yada yada.

