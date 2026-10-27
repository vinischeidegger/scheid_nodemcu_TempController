#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "${PROJECT_DIR}/.." && pwd)"

exec docker run --rm -it \
  -p 5000:5000 \
  -v "${WORKSPACE_DIR}:/workspace" \
  -w /workspace/scheid_nodemcu_TempController \
  python:3.10-slim \
  sh -c 'pip install --no-cache-dir flask flask-cors && python3 mock_server.py'
