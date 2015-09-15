IMAGE_PREFIX :=$(shell hostname)-$(shell id -u)-$(shell echo $$$$)-

DOCKER :=docker
docker_build =$(DOCKER) build --pull

DOCKER_NO_CACHE :=0
ifeq "$(DOCKER_NO_CACHE)" "1"
docker_build += --no-cache=true
endif
