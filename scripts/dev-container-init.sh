#!/usr/bin/bash

set -e

west init -l /home/lpl/clover
cd /home/lpl || exit 1
west update
uv pip install -r /home/lpl/zephyr/scripts/requirements.txt
west zephyr-export

# Set up flasherd
cd /home/lpl/clover || exit 1
cargo build --package flasherd-client --release
mkdir -p /home/lpl/clover/bin
cp -f /home/lpl/clover/target/release/flasherd-client /home/lpl/clover/bin/flasherd-client
