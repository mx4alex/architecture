PROJECT_NAME = budget_service
NPROCS ?= $(shell nproc)
CLANG_FORMAT ?= clang-format
DOCKER_IMAGE ?= ghcr.io/userver-framework/ubuntu-24.04-userver:latest
DOCKER_ARGS = $(shell /bin/test -t 0 && /bin/echo -it || echo)
PRESETS ?= debug release

.PHONY: all
all: test-debug test-release

# Run cmake
.PHONY: $(addprefix cmake-, $(PRESETS))
$(addprefix cmake-, $(PRESETS)): cmake-%:
	cmake --preset $*

$(addsuffix /CMakeCache.txt, $(addprefix build-, $(PRESETS))): build-%/CMakeCache.txt:
	$(MAKE) cmake-$*

# Build using cmake
.PHONY: $(addprefix build-, $(PRESETS))
$(addprefix build-, $(PRESETS)): build-%: build-%/CMakeCache.txt
	cmake --build build-$* -j $(NPROCS) --target $(PROJECT_NAME)

# Test
.PHONY: $(addprefix test-, $(PRESETS))
$(addprefix test-, $(PRESETS)): test-%: build-%/CMakeCache.txt
	cmake --build build-$* -j $(NPROCS)
	cd build-$* && ((test -t 1 && GTEST_COLOR=1 PYTEST_ADDOPTS="--color=yes" ctest -V) || ctest -V)

# Start the service
.PHONY: $(addprefix start-, $(PRESETS))
$(addprefix start-, $(PRESETS)): start-%:
	cmake --build build-$* -v --target start-$(PROJECT_NAME)

# Cleanup data
.PHONY: $(addprefix clean-, $(PRESETS))
$(addprefix clean-, $(PRESETS)): clean-%:
	cmake --build build-$* --target clean

.PHONY: dist-clean
dist-clean:
	rm -rf build*
	rm -rf tests/__pycache__/
	rm -rf tests/.pytest_cache/
	rm -rf .ccache

# Install
.PHONY: $(addprefix install-, $(PRESETS))
$(addprefix install-, $(PRESETS)): install-%: build-%
	cmake --install build-$* -v --component $(PROJECT_NAME)

.PHONY: install
install: install-release

# Format the sources
.PHONY: format
format:
	find src -name '*pp' -type f | xargs $(CLANG_FORMAT) -i

# Docker
.PHONY: docker-build docker-test docker-start
docker-%:
	docker run $(DOCKER_ARGS) \
		--platform linux/amd64 \
		-v $$PWD:$$PWD \
		-w $$PWD \
		$(DOCKER_IMAGE) \
		env CCACHE_DIR=$$PWD/.ccache \
		HOME=$$HOME \
		$$PWD/run_as_user.sh $(shell id -u) $(shell id -g) make $*

# Docker Compose
.PHONY: compose-up compose-down compose-logs
compose-up:
	docker compose up --build -d

compose-down:
	docker compose down -v

compose-logs:
	docker compose logs -f
