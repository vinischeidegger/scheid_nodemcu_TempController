#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./pio_remote_common.sh
source "${SCRIPT_DIR}/pio_remote_common.sh"

UPLOAD_FS="true"
UPLOAD_SPEEDS="${PIO_UPLOAD_SPEEDS:-57600 38400 19200}"
NODEMCU_HOST_PORT="${NODEMCU_HOST_PORT:-}"
COMPOSE_FILE_NO_DEVICES=""
COMPOSE_DEVICE_OVERRIDE=""

print_help() {
    cat <<'EOF'
Update NodeMCU remotely using PlatformIO inside Docker.

This script uploads firmware and LittleFS data by default.

Usage:
  scripts/pio_remote_update_nodemcu.sh [--firmware-only]

Flags:
  --firmware-only  Upload only firmware (skip filesystem upload).
  --with-fs        Explicitly upload filesystem after firmware (default behavior).
  -h, --help       Show this help message.

Optional environment variables:
  PIO_COMPOSE_FILE   Docker compose file (default: /srv/platformio/compose.yaml)
  PIO_SERVICE        Compose service name (default: platformio)
  PIO_CONTAINER_ROOT Container workspace root (default: /workspace)
  PIO_PROJECT_PATH   Project path from repo root (default: .)
  PIO_ENV            PlatformIO environment (default: nodemcuv2)
  NODEMCU_HOST_PORT  Host serial path (e.g. /dev/ttyUSB0). Auto-detected if unset.
  NODEMCU_PORT       Serial upload port (default: /dev/nodemcu)
  PIO_UPLOAD_SPEEDS  Space-separated retry speeds (default: "57600 38400 19200")
EOF
}

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

compose_run_upload() {
    docker compose -f "${COMPOSE_FILE_NO_DEVICES}" -f "${COMPOSE_DEVICE_OVERRIDE}" run --rm --no-deps "${PIO_SERVICE}" "$@"
}

upload_target_with_retries() {
    local target="$1"
    local label="$2"
    local -a speeds

    read -r -a speeds <<< "${UPLOAD_SPEEDS}"
    if [[ ${#speeds[@]} -eq 0 ]]; then
        echo "No upload speeds configured."
        return 1
    fi

    local attempt=1
    local total=${#speeds[@]}

    for speed in "${speeds[@]}"; do
        local host_tmp_conf="${REPO_ROOT}/.pio_upload_${target}_${speed}.ini"
        local container_tmp_conf="${CONTAINER_PROJECT_DIR}/.pio_upload_${target}_${speed}.ini"

        cp "${REPO_ROOT}/platformio.ini" "${host_tmp_conf}"
        if grep -qE '^upload_speed\s*=' "${host_tmp_conf}"; then
            sed -i -E "s/^upload_speed\s*=.*/upload_speed = ${speed}/" "${host_tmp_conf}"
        else
            printf '\nupload_speed = %s\n' "${speed}" >> "${host_tmp_conf}"
        fi

        echo "${label} attempt ${attempt}/${total} at ${speed} baud..."
        if compose_run_upload run -d "${CONTAINER_PROJECT_DIR}" --project-conf "${container_tmp_conf}" -e "${PIO_ENV}" -t "${target}" --upload-port "${NODEMCU_PORT}"; then
            rm -f "${host_tmp_conf}"
            echo "${label} succeeded at ${speed} baud."
            return 0
        fi

        rm -f "${host_tmp_conf}"
        echo "${label} failed at ${speed} baud."
        attempt=$((attempt + 1))
    done

    echo "${label} failed after ${total} attempts."
    return 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --with-fs)
            UPLOAD_FS="true"
            shift
            ;;
        --firmware-only)
            UPLOAD_FS="false"
            shift
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            print_help
            exit 1
            ;;
    esac
done

trap cleanup EXIT
prepare_compose_without_devices

if ! detect_host_serial_port; then
    echo "No serial device was found on host."
    echo "Set NODEMCU_HOST_PORT explicitly, for example:"
    echo "  NODEMCU_HOST_PORT=/dev/ttyUSB0 ./scheid_nodemcu_TempController/scripts/pio_remote_update_nodemcu.sh"
    exit 1
fi

prepare_device_override

print_runtime_config
echo "  upload port  : ${NODEMCU_PORT}"
echo "  host device  : ${NODEMCU_HOST_PORT}"
echo "  speeds       : ${UPLOAD_SPEEDS}"

echo
echo "Uploading firmware to NodeMCU..."
upload_target_with_retries "upload" "Firmware upload"

echo "Firmware upload finished successfully."

if [[ "${UPLOAD_FS}" == "true" ]]; then
    echo
    echo "Uploading filesystem image to NodeMCU..."
    upload_target_with_retries "uploadfs" "Filesystem upload"
    echo "Filesystem upload finished successfully."
else
    echo
    echo "Skipping filesystem upload (--firmware-only)."
fi

echo "NodeMCU update complete."
