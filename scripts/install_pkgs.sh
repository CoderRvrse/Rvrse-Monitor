#!/bin/bash
# Auto-install dependencies for Claude Code cloud sessions
echo "[Claude Setup] Installing Node packages..."
if [ -f package.json ]; then
  npm install --legacy-peer-deps
fi

echo "[Claude Setup] Installing Python dependencies..."
if [ -f requirements.txt ]; then
  pip install -r requirements.txt
fi

echo "[Claude Setup] Ready."
exit 0
