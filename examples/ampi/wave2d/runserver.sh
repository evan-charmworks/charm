#!/bin/sh
echo "Starting up server:"
./charmrun +p 4 ./wave2d +vp 64 ++server ++server-port 1234 ++local
echo "Server Exited"
