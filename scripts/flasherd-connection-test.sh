#!/bin/bash

# Does a dummy run of flasherd-client to ensure it can talk to host flasherd.

echo "[flasherd-connection-test] Testing flasherd..."
flasherd-client --command-windows echo --command-macos echo --command-linux echo --arg hello
status=$?

if [ $status -eq 0 ]; then
  echo "[flasherd-connection-test] flasherd is up!"
else
  echo "[flasherd-connection-test] flasherd test failed."
fi

exit $status
