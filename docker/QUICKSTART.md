# Quick Start Guide: Secure Containerized Cutter

This guide provides a quick reference for using the secure sandboxed Docker environments.

## Prerequisites

- Docker installed (Linux) or Docker Desktop (Windows/macOS)
- Make (GNU Make)
- Git (for cloning the repository)

## Quick Commands

### Build Secure Containers

```bash
# Linux secure container
cd docker
make build-secure-linux

# Windows secure container  
make build-secure-windows
```

### Run Securely

```bash
# Run with enhanced security protections
make run-secure
```

### Security Operations

```bash
# Run security audit
make security-audit-secure

# Apply security patches
make security-patch

# Scan with Trivy (if installed)
make security-scan-trivy
```

## Security Features Enabled

The secure containers include:

✅ **Non-root execution** - Runs as unprivileged user (UID 1000)  
✅ **Capability restrictions** - Minimal capabilities (only SYS_PTRACE)  
✅ **Read-only filesystem** - Root filesystem is read-only  
✅ **Resource limits** - Memory, CPU, and process limits enforced  
✅ **No privilege escalation** - Prevents privilege escalation attacks  
✅ **Health monitoring** - Built-in container health checks  
✅ **Security updates** - Latest patches applied during build  

## Comparison: Standard vs Secure

| Feature | Standard Container | Secure Container |
|---------|-------------------|------------------|
| Base Image Updates | Manual | Automatic during build |
| User Privileges | Root capable | Non-root enforced |
| Capabilities | Multiple | Minimal (SYS_PTRACE only) |
| Filesystem | Read-write | Read-only (with specific writable volumes) |
| Resource Limits | None | Memory, CPU, PID limits |
| Security Scanning | Manual | Automated with `make security-audit` |
| Patching | Manual | Automated with `make security-patch` |

## What Gets Scanned in Security Audit?

1. **CVE Vulnerabilities** - Known security vulnerabilities in packages
2. **Configuration Issues** - Insecure container configurations
3. **Privilege Escalation Risks** - Running as root, excessive capabilities
4. **Secrets Detection** - Accidentally embedded credentials
5. **Base Image Status** - Outdated or unpatched base images
6. **Best Practices** - Compliance with Docker security standards

## Maintenance Schedule

Recommended security maintenance tasks:

- **Weekly**: Run `make security-audit-secure`
- **Monthly**: Run `make security-patch`
- **After advisories**: Patch immediately for HIGH/CRITICAL CVEs

## Troubleshooting

### Build Fails

```bash
# Check Docker is running
docker ps

# Try without cache
sudo docker build --no-cache -f Dockerfile.secure-linux .
```

### Run Fails with Permission Errors

Ensure X11 forwarding is properly configured:
```bash
xhost +local:docker
```

### Security Audit Errors

Install Trivy for complete vulnerability scanning:
```bash
curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh
```

## Learn More

- Full documentation: [SECURITY.md](SECURITY.md)
- Main README: [README.md](README.md)
- Project security policy: [../SECURITY.md](../SECURITY.md)

## Getting Help

If you encounter issues:
1. Check the [SECURITY.md](SECURITY.md) troubleshooting section
2. Review audit reports in `audit-reports/`
3. Check patch logs in `patch-logs/`
4. Report security issues per [../SECURITY.md](../SECURITY.md)
