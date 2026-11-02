#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./pio_remote_common.sh
source "${SCRIPT_DIR}/pio_remote_common.sh"

UPLOAD_FS="true"
UPLOAD_SPEEDS="${PIO_UPLOAD_SPEEDS:-57600 38400 19200}"

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
  NODEMCU_PORT       Serial upload port (default: /dev/nodemcu)
    PIO_UPLOAD_SPEEDS  Space-separated retry speeds (default: "57600 38400 19200")
EOF
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
            if compose_run run -d "${CONTAINER_PROJECT_DIR}" --project-conf "${container_tmp_conf}" -e "${PIO_ENV}" -t "${target}" --upload-port "${NODEMCU_PORT}"; then
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

print_runtime_config
echo "  upload port  : ${NODEMCU_PORT}"
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
