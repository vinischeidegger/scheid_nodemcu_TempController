#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./pio_remote_common.sh
source "${SCRIPT_DIR}/pio_remote_common.sh"

MONITOR_BAUD="${MONITOR_BAUD:-115200}"
NODEMCU_HOST_PORT="${NODEMCU_HOST_PORT:-}"
COMPOSE_FILE_NO_DEVICES=""
COMPOSE_DEVICE_OVERRIDE=""

cleanup() {
  if [[ -n "${COMPOSE_FILE_NO_DEVICES}" ]]; then
    rm -f "${COMPOSE_FILE_NO_DEVICES}"
  fi
  if [[ -n "${COMPOSE_DEVICE_OVERRIDE}" ]]; then
    rm -f "${COMPOSE_DEVICE_OVERRIDE}"
  fi
}

prepare_compose_without_devices() {
  COMPOSE_FILE_NO_DEVICES="$(mktemp)"
  sed '/^[[:space:]]*devices:[[:space:]]*$/,/^[[:space:]]*volumes:[[:space:]]*$/ { /^[[:space:]]*volumes:[[:space:]]*$/!d }' "${PIO_COMPOSE_FILE}" > "${COMPOSE_FILE_NO_DEVICES}"
}

prepare_device_override() {
  COMPOSE_DEVICE_OVERRIDE="$(mktemp)"
  cat > "${COMPOSE_DEVICE_OVERRIDE}" <<EOF
services:
  ${PIO_SERVICE}:
    devices:
      - "${NODEMCU_HOST_PORT}:${NODEMCU_PORT}"
EOF
}

detect_host_serial_port() {
  if [[ -n "${NODEMCU_HOST_PORT}" ]]; then
    return 0
  fi

  if [[ -e "${NODEMCU_PORT}" ]]; then
    NODEMCU_HOST_PORT="${NODEMCU_PORT}"
    return 0
  fi

  if compgen -G "/dev/serial/by-id/*" > /dev/null; then
    NODEMCU_HOST_PORT="$(ls -1 /dev/serial/by-id/* | head -n 1)"
    return 0
  fi

  if compgen -G "/dev/ttyUSB*" > /dev/null; then
    NODEMCU_HOST_PORT="$(ls -1 /dev/ttyUSB* | head -n 1)"
    return 0
  fi

  if compgen -G "/dev/ttyACM*" > /dev/null; then
    NODEMCU_HOST_PORT="$(ls -1 /dev/ttyACM* | head -n 1)"
    return 0
  fi

  return 1
}

compose_run_monitor() {
  docker compose -f "${COMPOSE_FILE_NO_DEVICES}" -f "${COMPOSE_DEVICE_OVERRIDE}" run --rm --no-deps "${PIO_SERVICE}" "$@"
}

print_help() {
    cat <<'EOF'
Monitor NodeMCU serial output remotely using PlatformIO inside Docker.

Usage:
  scripts/pio_remote_monitor.sh

Optional environment variables:
  PIO_COMPOSE_FILE   Docker compose file (default: /srv/platformio/compose.yaml)
  PIO_SERVICE        Compose service name (default: platformio)
  NODEMCU_HOST_PORT  Host serial path (e.g. /dev/ttyUSB0). Auto-detected if unset.
  NODEMCU_PORT       Serial monitor port (default: /dev/nodemcu)
  MONITOR_BAUD       Serial baud rate (default: 115200)
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    print_help
    exit 0
fi

trap cleanup EXIT
prepare_compose_without_devices

if ! detect_host_serial_port; then
  echo "No serial device was found on host."
  echo "Set NODEMCU_HOST_PORT explicitly, for example:"
  echo "  NODEMCU_HOST_PORT=/dev/ttyUSB0 ./scheid_nodemcu_TempController/scripts/pio_remote_monitor.sh"
  exit 1
fi

prepare_device_override

print_runtime_config
echo "  monitor port : ${NODEMCU_PORT}"
echo "  host device  : ${NODEMCU_HOST_PORT}"
echo "  monitor baud : ${MONITOR_BAUD}"

echo
echo "Starting serial monitor (press Ctrl+C to stop)..."
compose_run_monitor device monitor --port "${NODEMCU_PORT}" --baud "${MONITOR_BAUD}"
