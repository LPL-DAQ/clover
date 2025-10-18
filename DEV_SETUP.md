# Development Environment

We use [Dev Containers](https://containers.dev/) for dependency management and consistent builds across all developer
machines. This handles all test and build of firmware, but to flash microcontrollers, we require a host daemon,
`flasherd` to cross the container barrier.

## `flasherd` installation

First, install Rust via [rustup](https://rustup.rs/).

Then, relaunch this repository in a dev container. Your IDE should automatically run a command to build and run
`flasherd`. IntelliJ on Windows doesn't do this correctly, so you may need to run it yourself from the `clover`
directory:

```shell
# On host
cargo run --bin flasherd_cleaner --release && cargo run --bin flasherd --release -- "c:\Users\james\lpl-flightsoft" --daemonize
```

**Example:**

```text
C:\Users\james\lpl-flightsoft>cargo run --bin flasherd_cleaner --release && cargo run --bin flasherd --release -- "c:\Users\james\lpl-flightsoft" --daemonize
   Compiling flasherd v0.1.0 (C:\Users\james\lpl-flightsoft\flasherd)
    Finished `release` profile [optimized] target(s) in 0.82s
     Running `target\release\flasherd_cleaner.exe`
[flasherd-cleaner] No flasherd matching pid 42768 found, is it already dead?
   Compiling flasherd v0.1.0 (C:\Users\james\lpl-flightsoft\flasherd)
    Finished `release` profile [optimized] target(s) in 3.42s
     Running `target\release\flasherd.exe c:\Users\james\lpl-flightsoft --daemonize`
Spawned flasherd with pid: 18152
```

If successful, you should see the following when you start a terminal in the dev container:

```text
   __   ___  __
  / /  / _ \/ /                                                                                                                                                                                                                                                                                                     
 / /__/ ___/ /__                                                                                                                                                                                                                                                                                                    
/____/_/  /____/                                                                                                                                                                                                                                                                                                    
Welcome!                                                                                                                                                                                                                                                                                                            
flasherd is active.
lpl@docker-desktop ~/clover Δ 
```

Afterwards, you can test your connection within the dev container by running:

```shell
flasherd-client --command-windows echo --command-macos echo --command-linux echo --arg hello
```

**Example:**

```
lpl@docker-desktop ~/clover Δ scripts/flasherd-connection-test.sh
[flasherd-connection-test] Testing flasherd...
[flasherd-client] Received args: ["--command-windows", "echo", "--command-macos", "echo", "--command-linux", "echo", "--arg", "hello"]
[flasherd-client] Connecting to host flasherd at port 6767
hello
[flasherd-client] Terminated naturally with status code 0
[flasherd-connection-test] flasherd is up!
```

## Test build

Run the following to build the `throttle` application:

```shell
west build -p auto throttle -b throttle_legacy
```

Then the following to flash:

```shell
west flash
```
