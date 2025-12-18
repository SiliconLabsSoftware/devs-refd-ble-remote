.SUFFIXES:				# ignore builtin rules
.PHONY: all clean transmitter_btl transmitter_app receiver receiver_on_host docker_image docker_build docker_run

TARGET ?= configure_build
TYPE ?= debug #release
PRJ_MACROS ?=

all: transmitter_btl transmitter_app receiver
transmitter_btl transmitter_app receiver:
	@echo 'Building $@...!'
	make -C projects -f sisdk_project_make.mak $(TARGET) TYPE=$(TYPE) CMAKELIST_DIR=$@/$@_cmake $(if $(PRJ_MACROS),PRJ_MACROS=$(PRJ_MACROS))
clean:
	@echo 'Cleaning all projects...!'
	make -C projects -f sisdk_project_make.mak clean TYPE=$(TYPE) CMAKELIST_DIR=receiver/receiver_cmake
	make -C projects -f sisdk_project_make.mak clean TYPE=$(TYPE) CMAKELIST_DIR=transmitter_btl/transmitter_btl_cmake
	make -C projects -f sisdk_project_make.mak clean TYPE=$(TYPE) CMAKELIST_DIR=transmitter_app/transmitter_app_cmake

PY_CMD ?= python3
PIP_CMD ?= pip3
PY_VIRTUAL_ENV_LOC ?= projects/receiver_on_host/.venv
PY_VIRTUAL_ENV_ACTIVATE ?= . ${PY_VIRTUAL_ENV_LOC}/bin/activate

receiver_on_host: venv
	@echo 'Starting application!'
	${PY_VIRTUAL_ENV_ACTIVATE} ; cd projects/receiver_on_host ; ${PY_CMD} main.py
venv: ${PY_VIRTUAL_ENV_LOC}/touchfile
${PY_VIRTUAL_ENV_LOC}/touchfile: projects/receiver_on_host/requirements.txt
	@echo 'Creating Python virtual environment...!'
	${PY_CMD} -m venv ${PY_VIRTUAL_ENV_LOC}
	${PY_VIRTUAL_ENV_ACTIVATE} ; ${PIP_CMD} install -r projects/receiver_on_host/requirements.txt
	touch ${PY_VIRTUAL_ENV_LOC}/touchfile

DOCKER_IMAGE_VERSION ?= latest
DOCKER_IMAGE_NAME ?= devs-refd-ble-remote-build-env:$(DOCKER_IMAGE_VERSION)
DOCKER_WORKSPACE ?= /home/ble-remote
ifeq ($(OS),Windows_NT)
	DOCKER_ARCH ?= x86_64
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
        DOCKER_ARCH ?= x86_64
    else
        DOCKER_ARCH ?= aarch64
    endif
endif
DOCKER_RUN_COMMON_ARGS := --platform linux/$(DOCKER_ARCH) --rm -v $(CURDIR):$(DOCKER_WORKSPACE) -w $(DOCKER_WORKSPACE) $(DOCKER_IMAGE_NAME)

docker_image:
	@echo 'Building Docker Image...!'
	docker build $(CURDIR) -t $(DOCKER_IMAGE_NAME) --platform linux/$(DOCKER_ARCH) --build-arg ARCH=$(DOCKER_ARCH)
docker_build:
	@echo 'Building project with the Docker Image...!'
	docker run $(DOCKER_RUN_COMMON_ARGS) /bin/sh -c "make all"
docker_run:
	@echo 'Running project interactively with the Docker Image...!'
	docker run -it $(DOCKER_RUN_COMMON_ARGS)
