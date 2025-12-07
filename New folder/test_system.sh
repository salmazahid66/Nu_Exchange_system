#!/bin/bash

# NU-Information Exchange System - Testing Script
# This script helps test the system by opening multiple terminals

echo "=========================================="
echo "  NU-Information Exchange System"
echo "  Automated Testing Script"
echo "=========================================="
echo ""

# Check if executables exist
if [ ! -f "./server" ]; then
    echo "[ERROR] Server executable not found. Run 'make' first."
    exit 1
fi

if [ ! -f "./client" ]; then
    echo "[ERROR] Client executable not found. Run 'make' first."
    exit 1
fi

echo "Starting Central Server (Islamabad)..."
gnome-terminal --title="Central Server - Islamabad" -- bash -c "./server; exec bash" &
sleep 2

echo "Starting Campus Clients..."

echo "  - Lahore Campus"
gnome-terminal --title="Campus Client - Lahore" -- bash -c "./client LAHORE NU-LHR-123; exec bash" &
sleep 1

echo "  - Karachi Campus"
gnome-terminal --title="Campus Client - Karachi" -- bash -c "./client KARACHI NU-KHI-123; exec bash" &
sleep 1

echo "  - Peshawar Campus"
gnome-terminal --title="Campus Client - Peshawar" -- bash -c "./client PESHAWAR NU-PWR-123; exec bash" &
sleep 1

echo "  - CFD Campus"
gnome-terminal --title="Campus Client - CFD" -- bash -c "./client CFD NU-CFD-123; exec bash" &
sleep 1

echo "  - Multan Campus"
gnome-terminal --title="Campus Client - Multan" -- bash -c "./client MULTAN NU-MLN-123; exec bash" &
sleep 1

echo ""
echo "=========================================="
echo "All terminals launched successfully!"
echo "=========================================="
echo ""
echo "Testing Instructions:"
echo "1. Wait for all clients to authenticate"
echo "2. In any client, choose option 1 to send a message"
echo "3. In server console, choose option 2 to broadcast"
echo "4. In any client, choose option 2 to view messages"
echo ""
echo "To stop all processes:"
echo "  pkill -f './server'"
echo "  pkill -f './client'"
echo ""
