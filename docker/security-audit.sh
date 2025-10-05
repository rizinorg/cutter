#!/bin/bash
# Security audit script for Docker containers
# Performs vulnerability scanning and security checks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="${1:-rizin/cutter}"
AUDIT_REPORT_DIR="${SCRIPT_DIR}/audit-reports"

echo "==================================="
echo "Docker Container Security Audit"
echo "==================================="
echo "Image: ${IMAGE_NAME}"
echo "Date: $(date)"
echo ""

# Create audit report directory
mkdir -p "${AUDIT_REPORT_DIR}"
REPORT_FILE="${AUDIT_REPORT_DIR}/security-audit-$(date +%Y%m%d-%H%M%S).txt"

# Function to log output
log() {
    echo "$1" | tee -a "${REPORT_FILE}"
}

log "Starting security audit for ${IMAGE_NAME}..."
log ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    log "ERROR: Docker is not installed or not in PATH"
    exit 1
fi

# Check if image exists
if ! docker image inspect "${IMAGE_NAME}" &> /dev/null; then
    log "ERROR: Image ${IMAGE_NAME} does not exist"
    exit 1
fi

# 1. Check for known vulnerabilities using Trivy (if available)
log "=== 1. Vulnerability Scanning ==="
if command -v trivy &> /dev/null; then
    log "Running Trivy vulnerability scan..."
    trivy image --severity HIGH,CRITICAL "${IMAGE_NAME}" | tee -a "${REPORT_FILE}"
else
    log "WARNING: Trivy not installed. Install with: curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh"
    log "Skipping vulnerability scan."
fi
log ""

# 2. Check Docker image history and layers
log "=== 2. Image Layer Analysis ==="
log "Analyzing image layers for security issues..."
docker history "${IMAGE_NAME}" --no-trunc | tee -a "${REPORT_FILE}"
log ""

# 3. Analyze image configuration
log "=== 3. Image Configuration Analysis ==="
CONFIG=$(docker inspect "${IMAGE_NAME}")

# Check if running as root
USER=$(echo "${CONFIG}" | grep -o '"User":"[^"]*"' | cut -d'"' -f4)
if [ -z "${USER}" ] || [ "${USER}" == "root" ] || [ "${USER}" == "0" ]; then
    log "WARNING: Container runs as root user. Consider using non-root user."
else
    log "PASS: Container runs as user: ${USER}"
fi

# Check for exposed ports
PORTS=$(echo "${CONFIG}" | grep -o '"ExposedPorts":{[^}]*}' || echo "")
if [ -n "${PORTS}" ]; then
    log "INFO: Exposed ports found: ${PORTS}"
else
    log "PASS: No ports exposed by default"
fi

# Check for health check
HEALTHCHECK=$(echo "${CONFIG}" | grep -o '"Healthcheck":{[^}]*}' || echo "")
if [ -n "${HEALTHCHECK}" ]; then
    log "PASS: Healthcheck configured"
else
    log "WARNING: No healthcheck configured"
fi
log ""

# 4. Check for secrets in image layers
log "=== 4. Secret Detection ==="
log "Scanning for potential secrets in image layers..."
if command -v docker-slim &> /dev/null; then
    docker-slim xray --target "${IMAGE_NAME}" | grep -i "secret\|password\|key\|token" | tee -a "${REPORT_FILE}" || log "No obvious secrets detected"
else
    log "INFO: docker-slim not available. Skipping detailed secret scan."
    log "Install docker-slim for enhanced secret detection: https://github.com/docker-slim/docker-slim"
fi
log ""

# 5. Check base image for security advisories
log "=== 5. Base Image Security ==="
BASE_IMAGE=$(docker inspect "${IMAGE_NAME}" | grep -o '"FROM [^"]*"' | head -1 | cut -d' ' -f2 | tr -d '"' || echo "unknown")
log "Base image: ${BASE_IMAGE}"
if [ "${BASE_IMAGE}" != "unknown" ]; then
    log "Checking if base image is up-to-date..."
    # Check if alpine version is latest
    if [[ "${BASE_IMAGE}" == *"alpine"* ]]; then
        log "Base image is Alpine Linux - checking for updates..."
        docker pull alpine:latest &> /dev/null || true
        CURRENT_ID=$(docker inspect "${BASE_IMAGE}" --format='{{.Id}}' 2>/dev/null || echo "")
        LATEST_ID=$(docker inspect alpine:latest --format='{{.Id}}' 2>/dev/null || echo "")
        if [ "${CURRENT_ID}" != "${LATEST_ID}" ] && [ -n "${CURRENT_ID}" ] && [ -n "${LATEST_ID}" ]; then
            log "WARNING: Base image may be outdated. Consider rebuilding with latest base image."
        else
            log "PASS: Base image appears current"
        fi
    fi
fi
log ""

# 6. Security best practices check
log "=== 6. Security Best Practices ==="

# Check image size (smaller is generally better for security)
SIZE=$(docker inspect "${IMAGE_NAME}" --format='{{.Size}}')
SIZE_MB=$((SIZE / 1024 / 1024))
log "Image size: ${SIZE_MB} MB"
if [ ${SIZE_MB} -gt 500 ]; then
    log "WARNING: Large image size (>${SIZE_MB}MB). Consider optimization to reduce attack surface."
else
    log "PASS: Image size is reasonable"
fi

# Check for package manager cache
log "Checking for leftover package manager cache..."
TEMP_CONTAINER=$(docker create "${IMAGE_NAME}")
docker export "${TEMP_CONTAINER}" | tar -tv | grep -E "var/cache|tmp/|var/tmp" | head -5 | tee -a "${REPORT_FILE}" || log "PASS: No obvious cache directories found"
docker rm "${TEMP_CONTAINER}" &> /dev/null
log ""

# 7. Runtime security recommendations
log "=== 7. Runtime Security Recommendations ==="
log "Recommended docker run flags for enhanced security:"
log "  --security-opt=no-new-privileges:true"
log "  --cap-drop=ALL"
log "  --cap-add=<only required capabilities>"
log "  --read-only (with specific writable volumes)"
log "  --tmpfs /tmp:rw,noexec,nosuid,size=128m"
log "  --memory=<limit>"
log "  --cpus=<limit>"
log "  --pids-limit=<limit>"
log "  --network=<isolated network>"
log ""

# Generate summary
log "==================================="
log "Audit Summary"
log "==================================="
log "Report saved to: ${REPORT_FILE}"
log ""
log "Review the audit report for security findings and recommendations."
log "Address any HIGH or CRITICAL vulnerabilities before deploying to production."
log ""

echo ""
echo "Security audit completed. Report: ${REPORT_FILE}"
