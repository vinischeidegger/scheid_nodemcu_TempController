# TempController (NodeMCU + DS18B20)

This project is a NodeMCU temperature controller base that uses:

- `ConfigManager` for WiFi + Captive Portal + persistent parameter storage
- `OneWire` and `DallasTemperature` for DS18B20 sensors

## Supported Topology

- Up to 6 DS18B20 sensors on one OneWire bus (`D2` by default)
- Per slot configuration in ConfigManager:
  - sensor name
  - sensor address (64-bit ROM in hex, e.g. `28FF4C601643037D`)
  - enabled/disabled

## How Configuration Works

At first boot:

- The firmware scans the OneWire bus.
- If a slot has an empty address and a sensor was discovered at the same index, the address is auto-filled and persisted.
- You can later override addresses and names through the ConfigManager web interface.

## Build

From this folder:

```bash
pio run
```

Upload filesystem (required for ConfigManager web assets):

```bash
pio run -t uploadfs
```

Upload firmware:

```bash
pio run -t upload
```

Serial monitor:

```bash
pio device monitor -b 115200
```

## Remote Docker Workflow (Homelab)

These scripts mirror the ConfigManager workflow and use Docker Compose PlatformIO.

- Build firmware:

```bash
./scripts/pio_remote_build.sh
```

- Upload firmware + filesystem:

```bash
./scripts/pio_remote_update_nodemcu.sh
```

- Upload firmware only:

```bash
./scripts/pio_remote_update_nodemcu.sh --firmware-only
```

- Monitor serial output:

```bash
./scripts/pio_remote_monitor.sh
```

Runtime overrides (optional):

```bash
PIO_COMPOSE_FILE=/srv/platformio/compose.yaml \
PIO_SERVICE=platformio \
PIO_CONTAINER_ROOT=/workspace \
PIO_PROJECT_PATH=. \
PIO_ENV=nodemcuv2 \
NODEMCU_PORT=/dev/nodemcu \
MONITOR_BAUD=115200 \
./scripts/pio_remote_update_nodemcu.sh
```

## Local Web UI Test (Without NodeMCU)

### Option A: Host Python (requires sudo once on this server)

If this host does not have pip and venv modules, install them first:

```bash
sudo apt update
sudo apt install -y python3-pip python3-venv
```

Then from this project folder:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install flask flask-cors
python mock_server.py
```

Open `http://localhost:5000` in your browser.

### Option B: Docker (no host Python setup required)

From this project folder:

```bash
./run_mock_server_docker.sh
```

Open `http://localhost:5000` in your browser.
