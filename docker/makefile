SHELL := /bin/bash

# The directory of this file
DIR := $(shell echo $(shell cd "$(shell  dirname "${BASH_SOURCE[0]}" )" && pwd ))

VERSION ?= latest
IMAGE_NAME ?= ps1337/cutter-docker
CONTAINER_NAME ?= cutter

# This will output the help for each task
# thanks to https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help

help: ## This help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DEFAULT_GOAL := help


# DOCKER TASKS
# Build the container
build: ## Build the container
	sudo docker build --rm -t $(IMAGE_NAME) .

build-nc: ## Build the container without caching
	sudo docker build --rm --no-cache -t $(IMAGE_NAME) .

run: ## Run container
	touch $(DIR)/radare2rc && \
	mkdir -p $(DIR)/r2-config && \
	mkdir -p $(DIR)/sharedFolder && \
	xhost +local:root && \
	sudo docker run \
	-it \
	--name $(CONTAINER_NAME) \
	--cap-drop=ALL  \
	--cap-add=SYS_PTRACE \
	-e DISPLAY=$$DISPLAY \
	-v /tmp/.X11-unix:/tmp/.X11-unix:ro \
	-v $(DIR)/sharedFolder:/var/sharedFolder \
	-v $(DIR)/radare2rc:/home/r2/.radare2rc \
	-v $(DIR)/r2-config:/home/r2/.config/radare2 \
	$(IMAGE_NAME):$(VERSION)

stop: ## Stop a running container
	sudo docker stop $(CONTAINER_NAME)

remove: ## Remove a (running) container
	sudo docker rm -f $(CONTAINER_NAME)

remove-image-force: ## Remove the latest image (forced)
	sudo docker rmi -f $(IMAGE_NAME):$(VERSION)

