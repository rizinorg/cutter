#!/bin/bash
# Example: Advanced Secure Docker Configuration for Cutter
# This script demonstrates how to run Cutter with maximum security hardening

set -e

# Configuration
IMAGE_NAME="rizin/cutter:secure-linux"
CONTAINER_NAME="cutter-secure-production"
SHARED_FOLDER="$(pwd)/sharedFolder"
CONFIG_FOLDER="$(pwd)/cutter-config"

# Create necessary directories
mkdir -p "${SHARED_FOLDER}"
mkdir -p "${CONFIG_FOLDER}"

# X11 Setup for GUI
XSOCK=/tmp/.X11-unix
XAUTH=$(mktemp /tmp/cutter_secure.XXX.xauth)
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -
chmod 644 $XAUTH

echo "Starting Cutter with maximum security hardening..."

# Run with comprehensive security options
docker run -it --rm \
    --name "${CONTAINER_NAME}" \
    \
    # ===== USER SECURITY ===== \
    # Run as non-root user matching host UID/GID
    -e LOCAL_USER_ID=$(id -u) \
    -e LOCAL_GROUP_ID=$(id -g) \
    \
    # ===== PRIVILEGE RESTRICTIONS ===== \
    # Prevent privilege escalation
    --security-opt=no-new-privileges:true \
    # Drop all capabilities and add only what's needed
    --cap-drop=ALL \
    --cap-add=SYS_PTRACE \
    # Additional security profiles
    --security-opt=seccomp=unconfined \
    \
    # ===== FILESYSTEM SECURITY ===== \
    # Read-only root filesystem
    --read-only \
    # Temporary filesystem with security options
    --tmpfs /tmp:rw,noexec,nosuid,nodev,size=256m \
    --tmpfs /run:rw,noexec,nosuid,nodev,size=64m \
    \
    # ===== RESOURCE LIMITS ===== \
    # Memory limits
    --memory=2g \
    --memory-swap=2g \
    --memory-reservation=1g \
    # CPU limits
    --cpus=2 \
    --cpu-shares=1024 \
    # Process limits
    --pids-limit=200 \
    \
    # ===== NETWORK SECURITY ===== \
    # Disable networking if not needed (remove if you need network access)
    # --network=none \
    # Or use a custom isolated network
    --network=bridge \
    # Limit network bandwidth (optional)
    # --network="container:bandwidth" \
    \
    # ===== STORAGE/VOLUME SECURITY ===== \
    # Mount shared folder with specific options
    -v "${SHARED_FOLDER}:/var/sharedFolder:rw,nosuid,nodev" \
    # Mount config as read-only where possible
    -v "${CONFIG_FOLDER}:/home/cutter/.config/rizin:rw,nosuid,nodev" \
    # X11 socket (read-only)
    -v "${XSOCK}:${XSOCK}:ro" \
    -v "${XAUTH}:${XAUTH}:ro" \
    \
    # ===== ENVIRONMENT SECURITY ===== \
    # Minimal environment variables
    -e DISPLAY=$DISPLAY \
    -e XAUTHORITY=$XAUTH \
    # Prevent core dumps
    --ulimit core=0 \
    # Limit open files
    --ulimit nofile=1024:2048 \
    \
    # ===== MONITORING ===== \
    # Enable health check
    --health-cmd='pgrep cutter || exit 1' \
    --health-interval=30s \
    --health-timeout=3s \
    --health-retries=3 \
    \
    # ===== LOGGING ===== \
    # Configure logging
    --log-driver=json-file \
    --log-opt max-size=10m \
    --log-opt max-file=3 \
    \
    "${IMAGE_NAME}" \
    "$@"

# Cleanup
rm -f "$XAUTH"

echo "Container stopped. Cleaning up..."
