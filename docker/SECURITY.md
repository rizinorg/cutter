# Docker Security Configuration

This document describes the secure sandboxing environments for Cutter Docker containers on both Linux and Windows platforms.

## Overview

The secure Docker configurations provide hardened containerized environments with:
- Security-focused base images with minimal attack surface
- Automated vulnerability scanning and patching
- Non-root user execution
- Capability restrictions
- Resource limitations
- Read-only filesystem where possible

## Secure Dockerfiles

### Linux Secure Container (`Dockerfile.secure-linux`)

The Linux secure container includes:
- **Minimal Base**: Alpine Linux with only required packages
- **Security Updates**: Automatic application of latest security patches during build
- **Non-Root User**: Runs as unprivileged `cutter` user (UID 1000)
- **Reduced Privileges**: Minimal capabilities, dropped unnecessary privileges
- **Health Monitoring**: Built-in healthcheck for container status
- **Clean Environment**: Removed caches and temporary files

### Windows Secure Container (`Dockerfile.secure-windows`)

The Windows secure container includes:
- **Windows Server Core**: Minimal Windows base with security updates
- **Latest Patches**: Automated Windows Update application
- **Non-Admin User**: Runs as standard user `cutteruser`
- **Access Controls**: Proper NTFS permissions on directories
- **Hardened Configuration**: Unnecessary Windows components removed

## Building Secure Containers

### Linux
```bash
cd docker
make build-secure-linux
```

### Windows
```bash
cd docker
make build-secure-windows
```

## Running Secure Containers

### Linux with Enhanced Security
```bash
cd docker
make run-secure
```

The `run-secure` target includes additional security flags:
- `--security-opt=no-new-privileges:true` - Prevents privilege escalation
- `--cap-drop=ALL` - Drops all capabilities
- `--cap-add=SYS_PTRACE` - Adds only required capabilities
- `--read-only` - Makes root filesystem read-only
- `--tmpfs /tmp:rw,noexec,nosuid,size=128m` - Temporary filesystem with security options
- `--memory=2g` - Memory limit
- `--cpus=2` - CPU limit
- `--pids-limit=100` - Process limit

### Manual Secure Run
```bash
docker run -it \
  --security-opt=no-new-privileges:true \
  --cap-drop=ALL \
  --cap-add=SYS_PTRACE \
  --read-only \
  --tmpfs /tmp:rw,noexec,nosuid,size=128m \
  --memory=2g \
  --cpus=2 \
  --pids-limit=100 \
  -v /path/to/shared:/var/sharedFolder \
  rizin/cutter:secure-linux
```

## Security Auditing

### Automated Security Audit

Run a comprehensive security audit on your container:

```bash
cd docker
make security-audit
```

Or for the secure image:
```bash
make security-audit-secure
```

The audit script (`security-audit.sh`) performs:
1. **Vulnerability Scanning**: Uses Trivy (if installed) to scan for known CVEs
2. **Layer Analysis**: Examines Docker layers for security issues
3. **Configuration Review**: Checks user privileges, exposed ports, healthchecks
4. **Secret Detection**: Scans for accidentally included secrets
5. **Base Image Check**: Verifies base image is up-to-date
6. **Best Practices**: Validates against Docker security best practices

### Installing Trivy for Enhanced Scanning

```bash
# Linux
curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin

# macOS
brew install aquasecurity/trivy/trivy

# Windows
choco install trivy
```

### Quick Trivy Scan

```bash
cd docker
make security-scan-trivy
```

## Security Patching

### Automated Patching

Apply the latest security patches by rebuilding with updated base images:

```bash
cd docker
make security-patch
```

For Windows:
```bash
make security-patch-windows
```

The patching script (`security-patch.sh`) performs:
1. **Updates Base Images**: Pulls latest base images with security patches
2. **Backup**: Creates backup of current image
3. **Rebuild**: Builds new image with `--no-cache` and `--pull` flags
4. **Verification**: Validates new image
5. **Audit**: Runs security audit on patched image
6. **Cleanup**: Removes dangling images

### Rollback Procedure

If issues are found after patching, rollback to the previous version:

```bash
# Find the backup image
docker images | grep backup

# Restore the backup
docker tag rizin/cutter:backup-TIMESTAMP rizin/cutter:secure-linux
```

## Security Best Practices

### 1. Regular Updates
- Run `make security-patch` weekly or after security advisories
- Monitor Alpine/Windows security mailing lists
- Subscribe to CVE alerts for container dependencies

### 2. Principle of Least Privilege
- Never run containers as root in production
- Add only required capabilities (e.g., SYS_PTRACE for debugging)
- Use read-only filesystems where possible

### 3. Resource Limits
- Always set memory limits (`--memory`)
- Set CPU limits (`--cpus`)
- Limit processes (`--pids-limit`)

### 4. Network Isolation
- Use custom networks instead of default bridge
- Implement network policies
- Expose only required ports

### 5. Secrets Management
- Never include secrets in images
- Use Docker secrets or external secret managers
- Mount secrets as files, not environment variables

### 6. Monitoring and Logging
- Enable container logging
- Monitor healthcheck status
- Set up alerts for security events

## Security Checklist

Before deploying to production:

- [ ] Run `make security-audit-secure`
- [ ] Review and address all HIGH/CRITICAL vulnerabilities
- [ ] Verify container runs as non-root user
- [ ] Test with minimal capabilities
- [ ] Enable read-only root filesystem
- [ ] Set resource limits
- [ ] Configure health checks
- [ ] Set up logging
- [ ] Document exposed ports and their purpose
- [ ] Implement network isolation
- [ ] Review mounted volumes and their permissions
- [ ] Test rollback procedure
- [ ] Set up automated patching schedule

## Continuous Security

### CI/CD Integration

Add security scanning to your CI/CD pipeline:

```yaml
# Example GitHub Actions workflow
- name: Security Scan
  run: |
    cd docker
    make security-audit-secure
    make security-scan-trivy
```

### Scheduled Patching

Set up automated patching with cron:

```bash
# Run weekly security patching
0 2 * * 0 cd /path/to/cutter/docker && make security-patch
```

## Compliance and Standards

These secure configurations align with:
- CIS Docker Benchmark
- NIST Application Container Security Guide
- Docker Security Best Practices
- OWASP Container Security Standard

## Reporting Security Issues

Security issues should be reported according to the project's [SECURITY.md](../SECURITY.md) policy.

Do not disclose security vulnerabilities publicly until they have been addressed.

## Additional Resources

- [Docker Security Documentation](https://docs.docker.com/engine/security/)
- [CIS Docker Benchmark](https://www.cisecurity.org/benchmark/docker)
- [NIST Container Security Guide](https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-190.pdf)
- [Trivy Documentation](https://aquasecurity.github.io/trivy/)
