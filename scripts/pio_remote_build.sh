#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=./pio_remote_common.sh
source "${SCRIPT_DIR}/pio_remote_common.sh"

print_help() {
    cat <<'EOF'
Build firmware remotely using PlatformIO inside Docker.

Usage:
  scripts/pio_remote_build.sh

Optional environment variables:
  PIO_COMPOSE_FILE   Docker compose file (default: /srv/platformio/compose.yaml)
  PIO_SERVICE        Compose service name (default: platformio)
  PIO_CONTAINER_ROOT Container workspace root (default: /workspace)
  PIO_PROJECT_PATH   Project path from repo root (default: .)
  PIO_ENV            PlatformIO environment (default: nodemcuv2)
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    print_help
    exit 0
fi

print_runtime_config

echo
echo "Building firmware in Docker..."

COMPOSE_FILE_NO_DEVICES="$(mktemp)"
sed '/^[[:space:]]*devices:[[:space:]]*$/,/^[[:space:]]*volumes:[[:space:]]*$/ { /^[[:space:]]*volumes:[[:space:]]*$/!d }' "${PIO_COMPOSE_FILE}" > "${COMPOSE_FILE_NO_DEVICES}"

cleanup() {
  rm -f "${COMPOSE_FILE_NO_DEVICES}"
}
trap cleanup EXIT

docker compose -f "${COMPOSE_FILE_NO_DEVICES}" run --rm --no-deps "${PIO_SERVICE}" run -d "${CONTAINER_PROJECT_DIR}" -e "${PIO_ENV}"

echo "Build finished successfully."
