build_type ?= Release
build_type_lower = ${shell echo ${build_type} | tr "[:upper:]" "[:lower:]"}

build_dir ?= build/${build_type_lower}
cmake_dir := $(CURDIR)

build_tests ?= OFF


all:
	@echo "Usage:"
	@echo "\tmake release"
	@echo "\tmake debug"
	@echo "\tmake docs"
	@echo "\tmake run"
	@echo "\tmake test"
	@echo "\tmake clean"
.PHONY: all


build:
	mkdir -p $(build_dir) && cd $(build_dir) && \
		cmake \
		-D CMAKE_BUILD_TYPE=$(build_type) \
		-D CPPHTML_ENABLE_TESTS=$(build_tests) \
		$(cmake_dir) && make
.PHONY: build


release:
	build_type=Release $(MAKE) build
.PHONY: release


debug:
	build_type=Debug CXX=clang++ $(MAKE) build
.PHONY: debug


test:
	build_tests=ON $(MAKE) build
	make -C $(build_dir) run-tests
.PHONY: test


clean:
	rm -rf build
.PHONY: clean


docs:
	mkdir -p $(build_dir)
	doxygen Doxyfile
.PHONY: docs


# Redirect all undefined targets to cmake.
%:
	make -C $(build_dir) $@
.PHONY:
