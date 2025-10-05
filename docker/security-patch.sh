#!/bin/bash
# Automated security patching script for Docker containers
# Rebuilds images with latest security patches

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="${1:-rizin/cutter}"
DOCKERFILE="${2:-Dockerfile.secure-linux}"
PATCH_LOG="${SCRIPT_DIR}/patch-logs/patch-$(date +%Y%m%d-%H%M%S).log"

echo "==================================="
echo "Docker Container Security Patching"
echo "==================================="
echo "Image: ${IMAGE_NAME}"
echo "Dockerfile: ${DOCKERFILE}"
echo "Date: $(date)"
echo ""

# Create log directory
mkdir -p "${SCRIPT_DIR}/patch-logs"

# Function to log output
log() {
    echo "$1" | tee -a "${PATCH_LOG}"
}

log "Starting security patching for ${IMAGE_NAME}..."
log ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    log "ERROR: Docker is not installed or not in PATH"
    exit 1
fi

# Check if Dockerfile exists
if [ ! -f "${SCRIPT_DIR}/${DOCKERFILE}" ]; then
    log "ERROR: Dockerfile ${DOCKERFILE} not found in ${SCRIPT_DIR}"
    exit 1
fi

# 1. Pull latest base images
log "=== 1. Updating Base Images ==="
log "Pulling latest base images to ensure security patches..."

# Extract base image from Dockerfile
BASE_IMAGES=$(grep -E "^FROM" "${SCRIPT_DIR}/${DOCKERFILE}" | awk '{print $2}' | sort -u)
for base in ${BASE_IMAGES}; do
    log "Pulling ${base}..."
    if docker pull "${base}"; then
        log "Successfully updated ${base}"
    else
        log "WARNING: Failed to pull ${base}"
    fi
done
log ""

# 2. Backup current image (if exists)
log "=== 2. Backing Up Current Image ==="
if docker image inspect "${IMAGE_NAME}" &> /dev/null; then
    BACKUP_TAG="${IMAGE_NAME}:backup-$(date +%Y%m%d-%H%M%S)"
    log "Creating backup of current image as ${BACKUP_TAG}..."
    docker tag "${IMAGE_NAME}" "${BACKUP_TAG}"
    log "Backup created: ${BACKUP_TAG}"
else
    log "No existing image to backup"
fi
log ""

# 3. Rebuild image with latest patches
log "=== 3. Rebuilding Image with Security Patches ==="
log "Building ${IMAGE_NAME} from ${DOCKERFILE}..."
BUILD_START=$(date +%s)

if docker build --no-cache --pull \
    -t "${IMAGE_NAME}" \
    -f "${SCRIPT_DIR}/${DOCKERFILE}" \
    "${SCRIPT_DIR}" 2>&1 | tee -a "${PATCH_LOG}"; then
    BUILD_END=$(date +%s)
    BUILD_TIME=$((BUILD_END - BUILD_START))
    log ""
    log "SUCCESS: Image rebuilt successfully in ${BUILD_TIME} seconds"
else
    BUILD_END=$(date +%s)
    BUILD_TIME=$((BUILD_END - BUILD_START))
    log ""
    log "ERROR: Image build failed after ${BUILD_TIME} seconds"
    log "Check the log at ${PATCH_LOG} for details"
    exit 1
fi
log ""

# 4. Verify new image
log "=== 4. Verifying New Image ==="
if docker image inspect "${IMAGE_NAME}" &> /dev/null; then
    IMAGE_ID=$(docker inspect "${IMAGE_NAME}" --format='{{.Id}}')
    IMAGE_CREATED=$(docker inspect "${IMAGE_NAME}" --format='{{.Created}}')
    IMAGE_SIZE=$(docker inspect "${IMAGE_NAME}" --format='{{.Size}}')
    IMAGE_SIZE_MB=$((IMAGE_SIZE / 1024 / 1024))
    
    log "Image ID: ${IMAGE_ID}"
    log "Created: ${IMAGE_CREATED}"
    log "Size: ${IMAGE_SIZE_MB} MB"
    log "PASS: New image verified successfully"
else
    log "ERROR: Failed to verify new image"
    exit 1
fi
log ""

# 5. Run security audit on new image
log "=== 5. Security Audit of Patched Image ==="
if [ -f "${SCRIPT_DIR}/security-audit.sh" ]; then
    log "Running security audit on newly patched image..."
    bash "${SCRIPT_DIR}/security-audit.sh" "${IMAGE_NAME}" 2>&1 | tee -a "${PATCH_LOG}"
else
    log "WARNING: security-audit.sh not found. Skipping automated audit."
    log "Manual security review recommended."
fi
log ""

# 6. Cleanup old images (optional)
log "=== 6. Cleanup ==="
log "Removing dangling images..."
DANGLING=$(docker images -f "dangling=true" -q)
if [ -n "${DANGLING}" ]; then
    docker rmi ${DANGLING} 2>&1 | tee -a "${PATCH_LOG}" || log "Some dangling images could not be removed"
    log "Cleanup completed"
else
    log "No dangling images to clean up"
fi
log ""

# Generate summary
log "==================================="
log "Patching Summary"
log "==================================="
log "Image: ${IMAGE_NAME}"
log "Dockerfile: ${DOCKERFILE}"
log "Build time: ${BUILD_TIME} seconds"
log "Log file: ${PATCH_LOG}"
log ""
log "Next steps:"
log "1. Review the security audit results"
log "2. Test the updated image thoroughly"
log "3. Deploy to staging environment"
log "4. After validation, deploy to production"
log ""
log "Rollback command (if needed):"
if [ -n "${BACKUP_TAG}" ]; then
    log "  docker tag ${BACKUP_TAG} ${IMAGE_NAME}"
fi
log ""

echo ""
echo "Security patching completed. Log: ${PATCH_LOG}"
