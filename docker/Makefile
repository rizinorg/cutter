SHELL := /bin/bash

# The directory of this file
DIR := $(shell echo $(shell cd "$(shell  dirname "${BASH_SOURCE[0]}" )" && pwd ))

# The local users UID/GID
LUID := $(shell id -u)
LGID := $(shell id -g)

# To mount a specific binary using BINARY=/absolute/path/to/binary
ifdef BINARY
    MOUNTFLAGS += -v $(BINARY):/home/cutter/$(shell basename $(BINARY)):ro
    RUNFLAGS += /home/cutter/$(shell basename $(BINARY))
endif

VERSION ?= latest
IMAGE_NAME ?= rizin/cutter
CONTAINER_NAME ?= cutter

# This will output the help for each task
# thanks to https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help

help: ## This help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DEFAULT_GOAL := help

# Build the container
build: ## Build the container
	sudo docker build --rm -t $(IMAGE_NAME) .

build-nc: ## Build the container without caching
	sudo docker build --rm --no-cache -t $(IMAGE_NAME) .

run: ## Run container
	XSOCK=/tmp/.X11-unix && \
	XAUTH=$(shell mktemp /tmp/cutter_tmp.XXX.xauth) && \
	xauth nlist $$DISPLAY | sed -e 's/^..../ffff/' | xauth -f $$XAUTH nmerge - && \
	chmod 644 $$XAUTH && \
	touch $(DIR)/rizinrc && \
	mkdir -p $(DIR)/cutter-config && \
	mkdir -p $(DIR)/sharedFolder && \
	sudo docker run \
		-it \
		--name $(CONTAINER_NAME) \
		--cap-add=SYS_PTRACE \
		-e DISPLAY=$$DISPLAY \
		-e XAUTHORITY=$$XAUTH \
		-e LOCAL_USER_ID=$(LUID) \
		-e LOCAL_GROUP_ID=$(LGID) \
		-v $$XSOCK:$$XSOCK:ro \
		-v $$XAUTH:$$XAUTH \
		$(MOUNTFLAGS) \
		-v $(DIR)/sharedFolder:/var/sharedFolder \
		-v $(DIR)/rizinrc:/home/cutter/.rizinrc \
		-v $(DIR)/cutter-config:/home/cutter/.config/rizin \
		$(IMAGE_NAME):$(VERSION) $(RUNFLAGS) && \
	rm $$XAUTH

get: ## Get the latest Cutter image
	sudo docker pull $(IMAGE_NAME):$(VERSION)

stop: ## Stop a running container
	sudo docker stop $(CONTAINER_NAME)

remove: ## Remove a (running) container
	sudo docker rm -f $(CONTAINER_NAME)

remove-image-force: ## Remove the latest image (forced)
	sudo docker rmi -f $(IMAGE_NAME):$(VERSION)

# Security targets
build-secure-linux: ## Build secure hardened Linux container
	sudo docker build --rm --no-cache -t $(IMAGE_NAME):secure-linux -f Dockerfile.secure-linux .

build-secure-windows: ## Build secure hardened Windows container
	sudo docker build --rm --no-cache -t $(IMAGE_NAME):secure-windows -f Dockerfile.secure-windows .

run-secure: ## Run container with enhanced security flags
	XSOCK=/tmp/.X11-unix && \
	XAUTH=$(shell mktemp /tmp/cutter_tmp.XXX.xauth) && \
	xauth nlist $$DISPLAY | sed -e 's/^..../ffff/' | xauth -f $$XAUTH nmerge - && \
	chmod 644 $$XAUTH && \
	touch $(DIR)/rizinrc && \
	mkdir -p $(DIR)/cutter-config && \
	mkdir -p $(DIR)/sharedFolder && \
	sudo docker run \
		-it \
		--name $(CONTAINER_NAME)-secure \
		--security-opt=no-new-privileges:true \
		--cap-drop=ALL \
		--cap-add=SYS_PTRACE \
		--read-only \
		--tmpfs /tmp:rw,noexec,nosuid,size=128m \
		--memory=2g \
		--cpus=2 \
		--pids-limit=100 \
		-e DISPLAY=$$DISPLAY \
		-e XAUTHORITY=$$XAUTH \
		-e LOCAL_USER_ID=$(LUID) \
		-e LOCAL_GROUP_ID=$(LGID) \
		-v $$XSOCK:$$XSOCK:ro \
		-v $$XAUTH:$$XAUTH \
		$(MOUNTFLAGS) \
		-v $(DIR)/sharedFolder:/var/sharedFolder \
		-v $(DIR)/rizinrc:/home/cutter/.rizinrc \
		-v $(DIR)/cutter-config:/home/cutter/.config/rizin \
		$(IMAGE_NAME):secure-linux $(RUNFLAGS) && \
	rm $$XAUTH

security-audit: ## Run security audit on container image
	@echo "Running security audit..."
	@bash security-audit.sh $(IMAGE_NAME):$(VERSION)

security-audit-secure: ## Run security audit on secure container image
	@echo "Running security audit on secure image..."
	@bash security-audit.sh $(IMAGE_NAME):secure-linux

security-patch: ## Apply security patches by rebuilding with latest base images
	@echo "Applying security patches..."
	@bash security-patch.sh $(IMAGE_NAME):secure-linux Dockerfile.secure-linux

security-patch-windows: ## Apply security patches to Windows container
	@echo "Applying security patches to Windows container..."
	@bash security-patch.sh $(IMAGE_NAME):secure-windows Dockerfile.secure-windows

security-scan-trivy: ## Scan image for vulnerabilities using Trivy
	@if command -v trivy >/dev/null 2>&1; then \
		echo "Scanning $(IMAGE_NAME):$(VERSION) with Trivy..."; \
		trivy image --severity HIGH,CRITICAL $(IMAGE_NAME):$(VERSION); \
	else \
		echo "Trivy not installed. Install with:"; \
		echo "  curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh"; \
	fi

