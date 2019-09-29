SHELL := /bin/bash

# The directory of this file
DIR := $(shell echo $(shell cd "$(shell  dirname "${BASH_SOURCE[0]}" )" && pwd ))

# The local users UID/GID
LUID := $(shell id -u)
LGID := $(shell id -g)

# To mount a specific binary using BINARY=/absolute/path/to/binary
ifdef BINARY
    MOUNTFLAGS += -v $(BINARY):/home/r2/$(shell basename $(BINARY)):ro
    RUNFLAGS += /home/r2/$(shell basename $(BINARY))
endif

VERSION ?= latest
IMAGE_NAME ?= radareorg/cutter
CONTAINER_NAME ?= cutter

LOCAL_DEV ?= n

# This will output the help for each task
# thanks to https://marmelab.com/blog/2016/02/29/auto-documented-makefile.html
.PHONY: help

help: ## This help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DEFAULT_GOAL := help

# Build the container
build: ## Build the container
	sudo docker build --rm -t $(IMAGE_NAME) $(if $(LOCAL_DEV) == 'y',-f ./Dockerfile-dev ../,.)

build-nc: ## Build the container without caching
	sudo docker build --rm --no-cache -t $(IMAGE_NAME) $(if $(LOCAL_DEV) == 'y',-f ./Dockerfile-dev ../,.)

run: ## Run container
	XSOCK=/tmp/.X11-unix && \
	XAUTH=$(shell mktemp /tmp/r2cutter_tmp.XXX.xauth) && \
	xauth nlist $$DISPLAY | sed -e 's/^..../ffff/' | xauth -f $$XAUTH nmerge - && \
	chmod 644 $$XAUTH && \
	touch $(DIR)/radare2rc && \
	mkdir -p $(DIR)/r2-config && \
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
		-v $(DIR)/radare2rc:/home/r2/.radare2rc \
		-v $(DIR)/r2-config:/home/r2/.config/radare2 \
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

