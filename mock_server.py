#!/usr/bin/env python3
"""
TempController local mock server.

Run from this folder:
    python3 mock_server.py

This serves ConfigManager web assets and mocks API responses using
TempController fields (6 DS18B20 slots).
"""

from pathlib import Path
import time

try:
    from flask import Flask, jsonify, request, send_from_directory
    from flask_cors import CORS
except ImportError:
    print("ERROR: Missing dependencies. Install with:")
    print("  pip install flask flask-cors")
    raise

BASE_DIR = Path(__file__).resolve().parent
CONFIG_MANAGER_WEB_SRC = (BASE_DIR / "../scheid_nodemcu_ConfigManager/web_src").resolve()

if not CONFIG_MANAGER_WEB_SRC.exists():
    raise SystemExit(
        "ERROR: Could not find ConfigManager web_src at "
        f"{CONFIG_MANAGER_WEB_SRC}.\n"
        "Check if ../scheid_nodemcu_ConfigManager exists."
    )

app = Flask(__name__)
CORS(app)

MOCK_ABOUT = {
    "page_title": "Temperature Controller Configuration",
    "device_name": "NodeMCU Temp Controller",
    "version": "0.1.0-local-mock",
}


def build_parameters():
    parameters = [
        {
            "id": "temp_read_interval_ms",
            "label": "Read Interval (ms)",
            "type": "ulong",
            "value": 5000,
        }
    ]

    for i in range(1, 7):
        default_enabled = i == 1
        parameters.append(
            {
                "id": f"sensor_{i}_name",
                "label": f"Sensor {i} Name",
                "type": "string",
                "value": f"Sensor {i}",
            }
        )
        parameters.append(
            {
                "id": f"sensor_{i}_address",
                "label": f"Sensor {i} Address",
                "type": "string",
                "value": "",
            }
        )
        parameters.append(
            {
                "id": f"sensor_{i}_enabled",
                "label": f"Sensor {i} Enabled",
                "type": "bool",
                "value": default_enabled,
            }
        )

    return parameters


MOCK_CONFIG = {
    "page_title": "Temperature Controller Configuration",
    "mdns_hostname": "temp-controller",
    "wifi_mode": "AP",
    "ap_ssid": "Scheid Config",
    "wifi_ssid": "",
    "parameters": build_parameters(),
}

MOCK_SSIDS = {
    "ssids": [
        {"ssid": "MyHomeNetwork", "rssi": -45, "channel": 6, "encryption": "WPA2"},
        {"ssid": "OfficeWiFi", "rssi": -60, "channel": 1, "encryption": "WPA2"},
        {"ssid": "LabGuest", "rssi": -70, "channel": 11, "encryption": "Open"},
    ]
}

MOCK_STATUS = [
    ["Device", "NodeMCU Temp Controller"],
    ["Mode", "AP"],
    ["Bus Pin", "D2"],
    ["Configured Sensor Slots", "6"],
    ["Read Interval", "5000 ms"],
]


@app.route("/")
def serve_index():
    return send_from_directory(str(CONFIG_MANAGER_WEB_SRC), "index.html")


@app.route("/<path:filename>")
def serve_static(filename):
    return send_from_directory(str(CONFIG_MANAGER_WEB_SRC), filename)


@app.route("/api/about")
def api_about():
    return jsonify(MOCK_ABOUT)


@app.route("/api/config")
def api_config():
    return jsonify(MOCK_CONFIG)


@app.route("/api/ssids")
def api_ssids():
    time.sleep(1)
    return jsonify(MOCK_SSIDS)


@app.route("/api/status")
def api_status():
    return jsonify(MOCK_STATUS)


@app.route("/api/connect", methods=["POST"])
def api_connect():
    if request.is_json:
        payload = request.get_json() or {}
        ssid = payload.get("ssid")
    else:
        ssid = request.form.get("ssid")

    if not ssid:
        return jsonify({"success": False, "error": "SSID is required."}), 400

    return jsonify({"success": True, "message": f"Connected to {ssid}."}), 200


@app.route("/api/return-to-ap", methods=["POST"])
def api_return_to_ap():
    return jsonify({"success": True, "message": "Restarting in AP mode..."}), 200


if __name__ == "__main__":
    print("=" * 70)
    print("TempController Mock API Server")
    print("=" * 70)
    print(f"Serving web assets from: {CONFIG_MANAGER_WEB_SRC}")
    print("Web UI: http://localhost:5000/")
    print("Config endpoint: http://localhost:5000/api/config")
    print("=" * 70)
    app.run(debug=True, host="localhost", port=5000)
