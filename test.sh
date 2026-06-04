#!/bin/bash
set -e
cd "$(dirname "$0")"
export PATH="$HOME/.local/bin:$PATH"
pip3 install pytest grpcio grpcio-tools --break-system-packages
python3 -m grpc_tools.protoc -I./cpp-application --python_out=. --grpc_python_out=. ./cpp-application/monitor.proto
pytest test_monitor.py -v
