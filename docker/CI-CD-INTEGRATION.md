# Docker Security CI/CD Integration Examples

This document provides examples for integrating Docker security scanning into CI/CD pipelines.

## GitHub Actions

Add this to `.github/workflows/docker-security.yml`:

```yaml
name: Docker Security Scan

on:
  push:
    branches: [ main, dev ]
    paths:
      - 'docker/**'
  pull_request:
    branches: [ main, dev ]
    paths:
      - 'docker/**'
  schedule:
    # Run weekly security scan
    - cron: '0 0 * * 0'

jobs:
  security-scan:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Build secure image
        run: |
          cd docker
          make build-secure-linux

      - name: Run Trivy vulnerability scan
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: 'rizin/cutter:secure-linux'
          format: 'sarif'
          output: 'trivy-results.sarif'
          severity: 'CRITICAL,HIGH'

      - name: Upload Trivy results to GitHub Security
        uses: github/codeql-action/upload-sarif@v3
        if: always()
        with:
          sarif_file: 'trivy-results.sarif'

      - name: Run custom security audit
        run: |
          cd docker
          make security-audit-secure

      - name: Upload audit report
        uses: actions/upload-artifact@v4
        if: always()
        with:
          name: security-audit-report
          path: docker/audit-reports/

      - name: Fail on HIGH/CRITICAL vulnerabilities
        run: |
          cd docker
          # Check if there are HIGH or CRITICAL vulnerabilities
          if grep -E "(HIGH|CRITICAL)" audit-reports/security-audit-*.txt; then
            echo "HIGH or CRITICAL vulnerabilities found!"
            exit 1
          fi
```

## GitLab CI

Add this to `.gitlab-ci.yml`:

```yaml
stages:
  - build
  - security

build-secure-image:
  stage: build
  image: docker:latest
  services:
    - docker:dind
  script:
    - cd docker
    - docker build -t cutter:secure-linux -f Dockerfile.secure-linux .
  only:
    - branches
    - tags

security-scan:
  stage: security
  image: docker:latest
  services:
    - docker:dind
  before_script:
    - apk add --no-cache curl bash make
    - curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin
  script:
    - cd docker
    - make build-secure-linux
    - trivy image --exit-code 1 --severity CRITICAL,HIGH cutter:secure-linux
    - make security-audit-secure
  artifacts:
    paths:
      - docker/audit-reports/
    expire_in: 1 week
  only:
    - branches
    - merge_requests

scheduled-security-patch:
  stage: security
  image: docker:latest
  services:
    - docker:dind
  script:
    - cd docker
    - make security-patch
  only:
    - schedules
```

## Jenkins Pipeline

Add this to `Jenkinsfile`:

```groovy
pipeline {
    agent any
    
    environment {
        DOCKER_IMAGE = 'rizin/cutter:secure-linux'
    }
    
    stages {
        stage('Build Secure Image') {
            steps {
                dir('docker') {
                    sh 'make build-secure-linux'
                }
            }
        }
        
        stage('Security Scan - Trivy') {
            steps {
                script {
                    sh '''
                        trivy image --exit-code 0 --severity LOW,MEDIUM ${DOCKER_IMAGE}
                        trivy image --exit-code 1 --severity HIGH,CRITICAL ${DOCKER_IMAGE}
                    '''
                }
            }
        }
        
        stage('Security Audit') {
            steps {
                dir('docker') {
                    sh 'make security-audit-secure'
                }
            }
            post {
                always {
                    archiveArtifacts artifacts: 'docker/audit-reports/**/*.txt'
                }
            }
        }
        
        stage('Security Gate') {
            steps {
                script {
                    def auditResults = sh(
                        script: 'grep -c "CRITICAL\\|HIGH" docker/audit-reports/security-audit-*.txt || echo 0',
                        returnStdout: true
                    ).trim()
                    
                    if (auditResults.toInteger() > 0) {
                        error "Security gate failed: HIGH or CRITICAL vulnerabilities found!"
                    }
                }
            }
        }
    }
    
    post {
        always {
            cleanWs()
        }
    }
}
```

## Azure DevOps

Add this to `azure-pipelines.yml`:

```yaml
trigger:
  branches:
    include:
      - main
      - dev
  paths:
    include:
      - docker/*

schedules:
  - cron: "0 0 * * 0"
    displayName: Weekly security scan
    branches:
      include:
        - main
    always: true

pool:
  vmImage: 'ubuntu-latest'

stages:
  - stage: Build
    jobs:
      - job: BuildSecure
        steps:
          - task: Docker@2
            displayName: Build secure Linux image
            inputs:
              command: build
              Dockerfile: docker/Dockerfile.secure-linux
              tags: secure-linux

  - stage: SecurityScan
    dependsOn: Build
    jobs:
      - job: TrivyScan
        steps:
          - script: |
              curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin
              trivy image --exit-code 1 --severity CRITICAL,HIGH rizin/cutter:secure-linux
            displayName: Run Trivy scan

      - job: CustomAudit
        steps:
          - script: |
              cd docker
              make security-audit-secure
            displayName: Run security audit
          
          - task: PublishBuildArtifacts@1
            inputs:
              pathToPublish: docker/audit-reports
              artifactName: security-audit-reports
            condition: always()
```

## CircleCI

Add this to `.circleci/config.yml`:

```yaml
version: 2.1

orbs:
  docker: circleci/docker@2.0.1

jobs:
  build-secure:
    docker:
      - image: docker:latest
    steps:
      - checkout
      - setup_remote_docker
      - run:
          name: Build secure image
          command: |
            cd docker
            make build-secure-linux

  security-scan:
    docker:
      - image: docker:latest
    steps:
      - checkout
      - setup_remote_docker
      - run:
          name: Install Trivy
          command: |
            apk add --no-cache curl bash
            curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin
      - run:
          name: Run security scans
          command: |
            cd docker
            make build-secure-linux
            trivy image --exit-code 1 --severity CRITICAL,HIGH rizin/cutter:secure-linux
            make security-audit-secure
      - store_artifacts:
          path: docker/audit-reports
          destination: security-reports

workflows:
  version: 2
  build-and-scan:
    jobs:
      - build-secure
      - security-scan:
          requires:
            - build-secure
  
  scheduled-scan:
    triggers:
      - schedule:
          cron: "0 0 * * 0"
          filters:
            branches:
              only:
                - main
    jobs:
      - security-scan
```

## Best Practices for CI/CD Security Integration

1. **Fail Fast**: Configure pipelines to fail on HIGH/CRITICAL vulnerabilities
2. **Regular Scans**: Schedule weekly or daily security scans
3. **Artifact Storage**: Always save security reports as artifacts
4. **Branch Protection**: Require security scans to pass before merging
5. **Notifications**: Configure alerts for security findings
6. **Auto-Patching**: Consider automated patching for low-risk updates
7. **Rollback Plan**: Always test and have rollback procedures

## Security Scanning Tools Integration

### Trivy
```bash
# Install
curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh

# Scan
trivy image --severity HIGH,CRITICAL rizin/cutter:secure-linux
```

### Anchore
```bash
# Install
pip install anchorecli

# Scan
anchore-cli image add rizin/cutter:secure-linux
anchore-cli image wait rizin/cutter:secure-linux
anchore-cli image vuln rizin/cutter:secure-linux os
```

### Clair
```bash
# Requires Clair server running
clairctl analyze rizin/cutter:secure-linux
```

### Snyk
```bash
# Install
npm install -g snyk

# Scan
snyk container test rizin/cutter:secure-linux
```

## Continuous Monitoring

Set up continuous monitoring using:
- **Prometheus**: For metrics collection
- **Grafana**: For visualization
- **Falco**: For runtime security monitoring
- **ELK Stack**: For log aggregation and analysis

## Compliance Reporting

Generate compliance reports for:
- CIS Docker Benchmark
- PCI-DSS
- HIPAA
- SOC 2

Example using Docker Bench for Security:
```bash
docker run -it --net host --pid host --userns host --cap-add audit_control \
  -v /var/lib:/var/lib \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v /etc:/etc --label docker_bench_security \
  docker/docker-bench-security
```
