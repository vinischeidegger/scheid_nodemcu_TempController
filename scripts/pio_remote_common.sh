#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_NAME="$(basename "${REPO_ROOT}")"

PIO_COMPOSE_FILE="${PIO_COMPOSE_FILE:-/srv/platformio/compose.yaml}"
PIO_SERVICE="${PIO_SERVICE:-platformio}"
PIO_CONTAINER_ROOT="${PIO_CONTAINER_ROOT:-/workspace}"
PIO_PROJECT_PATH="${PIO_PROJECT_PATH:-.}"
PIO_ENV="${PIO_ENV:-nodemcuv2}"
NODEMCU_PORT="${NODEMCU_PORT:-/dev/nodemcu}"

CONTAINER_REPO_DIR="${PIO_CONTAINER_ROOT}/${REPO_NAME}"
CONTAINER_PROJECT_DIR="${CONTAINER_REPO_DIR}/${PIO_PROJECT_PATH}"

compose_run() {
    docker compose -f "${PIO_COMPOSE_FILE}" run --rm --no-deps "${PIO_SERVICE}" "$@"
}

print_runtime_config() {
    echo "Remote PlatformIO configuration:"
    echo "  compose file : ${PIO_COMPOSE_FILE}"
    echo "  service      : ${PIO_SERVICE}"
    echo "  repo root    : ${REPO_ROOT}"
    echo "  container dir: ${CONTAINER_PROJECT_DIR}"
    echo "  env          : ${PIO_ENV}"
}
