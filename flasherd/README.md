# flasherd

Lightweight daemon that allows flashing of MCUs from within a container. The flasher CLI
within the container is replaced with a client that sends its arguments through a TCP
socket to this daemon, which calls the real device flasher on the host OS. Paths
relative to the container root are dynamically replaced to be relative to the host root.

## Distribution

As flasherd is an integral part of the development environment, its release is tied with
that of the dev container image. The script at `/scripts/publish-dev-container.sh` will
take care of compiling the binaries and placing them in `/bin/flasherd/*`. Because these
binaries are committed into the repo alongside the image digest change in `/devcontainers.json`,
developers pulling to update to a given container version will also update their flasherd binaries.

## Protocol

### Request

The client should establish a TCP connection on port `TBD`. To send a command, the client
should send a little-endian u32 declaring the number of arguments to come, then a null
character. Then, each argument should be a null-terminated string sent sequentially. 
The u32 header should match the number of subsequent arguments. The first argument should
be the name of the executable to run, which must be on a pre-defined whitelist.

Here's an example request:

```
0x00 0x00 0x00 0x04 0x00    4 incoming arguments, null seperator
"stm32cubeprogrammer\0"     What to execute, must be whitelisted
"--board\0"                 Second arg
"lpl-test-board\0"          Third arg
"--some-other-arg\0"        Fourth arg
```

Once the request is complete, the client may send additional requests sequentially.

### Resposne

After receiving a request, flasherd will send lines of output to stream the result of the
requested command. Each line will begin with a 2-byte header, then between 0 and 2^14-1 bytes of content.

```
┌────────┬────────┐                               
│PPLLLLLL│LLLLLLLL│                               
└▲─▲─────┴────────┘                               
 │ └────Length of incoming content, network-endian
 │                                                
 └────2-bit prefix                                
 ```

|Prefix|Meaning|
|-|-|
|`00`|Line of stdout|
|`01`|Line of stderr|
|`10`|Program has terminated. Line will be one byte containing the status code as a u8.|
|`11`|flasherd had an error, perhaps malformed input or something internal. Content line will be an error message.|

If a line has a `1X` prefix, do not expect any additional lines to follow -- the response has fully ended.
The client, emulating a real execution of the CLI, should thus terminate.
