ifeq ($(MAKECMDGOALS),)
    $(error No targets given!)
endif

CXX ?= g++
ifeq ($(CC), gcc)
	CXX := g++
endif

ifeq ($(CC), clang)
	CXX := clang++
	COMPILE_TIME_TRACE = -ftime-trace
endif

$(info CXX is set to $(CXX))
PRJ_NAME := FootMadeHero

### Paths ###
PRJ_ROOT        := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
PRJ_SOURCE_CODE := $(PRJ_ROOT)/src
PRJ_OUTPUT_DIR  := $(PRJ_ROOT)/Outputs
PRJ_OBJECTS_DIR := $(PRJ_OUTPUT_DIR)/Objects
EXECUTABLE      := $(PRJ_OUTPUT_DIR)/$(PRJ_NAME).elf

DEPDIR := $(PRJ_OUTPUT_DIR)/.deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

git_hash := $(shell git describe --always --dirty)
git_hash_full := $(shell git rev-parse HEAD)
user_name := $(shell whoami)
machine_name = $(shell hostname)

USER_INCLUDE_DIRS := \
	$(PRJ_SOURCE_CODE) \
	$(PRJ_SOURCE_CODE)/third.party/tl/optional/include \
	$(PRJ_SOURCE_CODE)/third.party/tl/expected/include \
	$(PRJ_SOURCE_CODE)/third.party/strong_type/include \
	$(PRJ_SOURCE_CODE)/third.party/swl_variant/include \
	$(PRJ_SOURCE_CODE)/third.party/unordered_dense/include \
	$(PRJ_SOURCE_CODE)/third.party/cpp-lazy/include \
	$(PRJ_SOURCE_CODE)/third.party/libconfig/include \
	$(PRJ_SOURCE_CODE)/third.party/KHR/include \
	$(PRJ_SOURCE_CODE)/third.party/ktx/include \
	$(PRJ_SOURCE_CODE)/third.party/mio/single_include \
	$(PRJ_SOURCE_CODE)/third.party/spirv-reflect \
	$(PRJ_SOURCE_CODE)/third.party/tiny_gltf \
	$(PRJ_SOURCE_CODE)/third.party/stb/include \
	$(PRJ_SOURCE_CODE)/third.party/basis_universal \
	$(PRJ_SOURCE_CODE)/third.party/imgui-docking/include \
	$(PRJ_SOURCE_CODE)/third.party/imgui-docking/include/imgui \
	$(PRJ_SOURCE_CODE)/third.party/concurren-cpp/include \
	$(PRJ_SOURCE_CODE)/third.party/jolt-physics \
	$(PRJ_SOURCE_CODE)/third.party/frozen/include \
	$(PRJ_SOURCE_CODE)/third.party/libnoise/include \
	$(PRJ_SOURCE_CODE)/third.party/atomic_queue/include \

PROBLEMATIC_SOURCE_FILES = \
	$(PRJ_SOURCE_CODE)/xray/math/delaunay.triangulator.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/geometry/aabb_visualizer.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/geometry/surface_normal_visualizer.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/user_interface_backend_opengl.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/app_window.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/build-info-tool.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/crash.sentinel.cpp \
	$(PRJ_SOURCE_CODE)/game/code/stargen/stargen.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/platform.window.win32.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/mesh.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/mesh_loader.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/ui.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.atmosphere.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.dust.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.gas.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.helpers.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.planet.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.solar.system.cc \
	$(PRJ_SOURCE_CODE)/xray/stargen/stargen.star.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/test.bda.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/game/game.sim.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/terrain.cc

SOURCE_FILES = \
	$(PRJ_SOURCE_CODE)/xray/base/serialization/serialization.cc \
	$(PRJ_SOURCE_CODE)/xray/base/app_config.cc \
	$(PRJ_SOURCE_CODE)/xray/base/config_settings.cc \
	$(PRJ_SOURCE_CODE)/xray/base/file_system_watcher.cc \
	$(PRJ_SOURCE_CODE)/xray/base/logger.cc \
	$(PRJ_SOURCE_CODE)/xray/base/memory.pool.cc \
	$(PRJ_SOURCE_CODE)/xray/base/perf_stats_collector.cc \
	$(PRJ_SOURCE_CODE)/xray/base/thread.local.context.cc \
	$(PRJ_SOURCE_CODE)/xray/base/xray.debug.cc \
	$(PRJ_SOURCE_CODE)/xray/base/xray.os.cc \
	$(PRJ_SOURCE_CODE)/xray/base/memory.arena.cc \
	$(PRJ_SOURCE_CODE)/xray/base/xray.fmt.cc \
	$(PRJ_SOURCE_CODE)/xray/base/xray.fmt.arena.cc \
	$(PRJ_SOURCE_CODE)/xray/physics/particle.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/colors/color_cast_rgb_hsl.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/colors/color_cast_rgb_hsv.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/colors/color_cast_rgb_xyz.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/colors/color_palettes.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/colors/rgb_color.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/geometry/geometry_factory.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/geometry/heightmap.generator.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/shapes.system/shapes.system.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/sprite.system/sprite.system.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.bindless.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.buffer.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.error.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.image.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.pipeline.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.renderer.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/vulkan.renderer/vulkan.renderer.config.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/debug_draw.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/geometry.importer.gltf.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/procedural.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/render_stage.cc \
	$(PRJ_SOURCE_CODE)/xray/rendering/shader.code.builder.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/camera.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/camera.controller.arcball.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/camera.controller.flight.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/camera_controller_spherical_coords.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/fps_camera_controller.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/scene.definition.cc \
	$(PRJ_SOURCE_CODE)/xray/scene/scene.description.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/events.pretty.print.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/platform.window.x11.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/user.interface.backend.vulkan.cc \
	$(PRJ_SOURCE_CODE)/xray/ui/user_interface.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/blue.noise.poisson.disk.sampler.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/flight.cam.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/system.memory.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/system.physics.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/system.physics.debug.renderer.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/game.main.cc \
	$(PRJ_SOURCE_CODE)/xray/game/code/serialize.inst.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.01.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.02.c \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.03.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.05.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.09.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.10.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.00.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.04.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.06.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.07.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.08.cc \
	$(PRJ_SOURCE_CODE)/app/bulk.build.xray.11.cc

VPATH = $(sort $(dir $(SOURCE_FILES)))
# $(info VPATH set to $(VPATH))

### Exported Variables ###
# Variables exported for user-defined libraries
export ARCH_TYPE ?= x86_64
export DEFINES
export CPP_FLAGS
export SANITIZERS
export SYSTEM_ROOT_PATH

ifneq ($(MAKECMDGOALS),clean)
    $(info Compiling the project '$(PRJ_NAME)')

    ### Architecture Selection ###
    ifeq ($(ARCH_TYPE), x86_64)
        $(info x86 64-bit architecture choosen!)

        SYSTEM_ROOT_PATH := /
    else ifeq ($(ARCH_TYPE), aarch64)
        $(info ARM 64-bit architecture choosen!)

        CC := aarch64-linux-gnu-g++
        SZ := aarch64-linux-gnu-size

        SYSTEM_ROOT_PATH := /path/to/aarch64-rootfs
    else ifeq ($(ARCH_TYPE), arm)
        $(info ARM 32-bit architecture choosen!)

        CC := arm-linux-gnueabihf-g++
        SZ := arm-linux-gnueabihf-size

        SYSTEM_ROOT_PATH := /path/to/arm-rootfs
    else
        $(error Architecture type not recognized! ARCH_TYPE: '$(ARCH_TYPE)')
    endif

    # Check if the toolchain exist in the environment
    ifeq ($(strip $(shell which $(CC))),)
        $(error Toolchain($(CC)) not found in the path! Check the environment.)
    endif

    ### Symbols ###
    DEFINES += -DLINUX_OS \
		-DXRAY_GRAPHICS_API_VULKAN \
		-D_XOPEN_SOURCE=700 \
		-D_GNU_SOURCE \
		-DXRAY_BUILD_GIT_HASH="\"$(git_hash)\"" \
		-DXRAY_BUILD_GIT_HASH_FULL="\"$(git_hash_full)\"" \
		-DXRAY_USER_NAME="\"$(user_name)\"" \
		-DXRAY_MACHINE_NAME="\"$(machine_name)\""

    ### Compilation Flags ###
    DEBUG_LEVEL         := 3
    OPTIMIZATION_LEVEL  := 0
	CPP_STD				:= -std=c++26 -freflection
    CPP_FLAGS           := -O$(OPTIMIZATION_LEVEL) -g$(DEBUG_LEVEL) -Wall $(CPP_STD) -mavx2 -mbmi -mpopcnt -mlzcnt -mf16c
	LD_FLAGS			:= -fuse-ld=mold -lpthread -latomic

	ifeq ($(CC),clang++)
		CPP_FLAGS += -stdlib=libc++
		LD_FLAGS += -lstdc++
	endif

	LD_FLAGS += -lstdc++exp

    ### Sanitizers ###
    ifeq ($(DEBUG_LEVEL), 3)
        ifeq ($(OPTIMIZATION_LEVEL), 0)
            $(warning Enabling the sanitizers..)

            # Detect memory related errors
            SANITIZERS += -fsanitize=address

            # Detect undefined behavior
            # SANITIZERS += -fsanitize=undefined

            # Detect memory leaks
            # SANITIZERS += -fsanitize=leak

            # Detect thread safety issues
            # SANITIZERS += -fsanitize=thread

            # Detect overflows
            # SANITIZERS += -fsanitize=signed-integer-overflow

            # Detect out-of-bound accesses
            # SANITIZERS += -fsanitize=bounds
        endif
    endif

    ### External Libraries ###
	LD_FLAGS += -lvulkan \
		-lshaderc \
		-lshaderc_combined \
		-lSPIRV-Tools-opt \
		-lSPIRV-Tools \
		-lSPIRV-Tools-link \
		-lglslang \
		-lzstd

	LD_FLAGS += -lX11 \
		-lXi \
		-lXinerama \
		-lxkbcommon-x11 \
		-lxkbcommon \
		-lX11-xcb \
		-lxcb

    ### Include paths ###
    # External libraries
    INCLUDES += -I$(SYSTEM_ROOT_PATH)usr/include

    # User-defined libraries
    INCLUDES += $(foreach includeDirName,$(USER_INCLUDE_DIRS),-I$(includeDirName))

    ### Derived Variables ###
	SRC_FILES_CLEAN = $(notdir $(SOURCE_FILES))
	OBJECT_FILES += $(addprefix $(PRJ_OBJECTS_DIR)/, $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.cxx,%.o,$(SRC_FILES_CLEAN))))))

    # Dependency files
    DEP_FILES       = $(patsubst $(PRJ_OBJECTS_DIR)/%.o,$(DEPDIR)/%.d, $(OBJECT_FILES))        # All dependency files
endif

TIMECMD := $(shell command -v time 2>/dev/null || echo -n '')
$(info Time command is $(TIMECMD))
# Conditional usage
ifneq ($(TIMECMD),)
	COMPILE.time = $(TIMECMD) -f "Compile took: %E"
	LINK.time = $(TIMECMD) -f "Link took: %E"
else
	COMPILE.time =
	LINK.time =
endif

COMPILE.cpp = $(COMPILE.time) $(CXX) $(COMPILE_TIME_TRACE) $(DEPFLAGS) $(CPP_FLAGS) $(SANITIZERS) $(INCLUDES) $(DEFINES) -c
COMPILE.c = $(COMPILE.time) $(CC) -x c $(COMPILE_TIME_TRACE) -std=c11 $(DEPFLAGS) $(SANITIZERS) $(INCLUDES) $(DEFINES) -c

cfg_file = $(PRJ_OUTPUT_DIR)/config/app.config.conf

config_step:
	@mkdir -p $(dir $(cfg_file))
	@printf "directories: {\nroot_win = \"c:/games/xray\";" > $(cfg_file)
	@printf "\nroot = \"/home/%s/Games/xray\";" "$(shell whoami)" >> $(cfg_file)
	@printf "\nmodels = \"assets/models\";" >> $(cfg_file)
	@printf "\nfonts = \"assets/fonts\";" >> $(cfg_file)
	@printf "\ntextures = \"assets/textures\";" >> $(cfg_file)
	@printf "\nshaders = \"%s\";" "$(shell pwd)/src/xray/game/shaders" >> $(cfg_file)
	@printf "\n};" >> $(cfg_file)
	@echo "$(cfg_file) created"

compile_txt = $(PRJ_ROOT)/compile_flags.txt
compile_txt_step:$(compile_txt)
	@echo "-std=c++23\n-Wall\n-Wextra" > $(compile_txt)
	@printf -- "%s\n" $(INCLUDES) >> $(compile_txt)
	@printf -- "%s\n" $(DEFINES) >> $(compile_txt)
	@echo "Wrote $(compile_txt)"

### Targets ###
# Complete build (Compilation and linking)
all: $(OBJECT_FILES) config_step
	@echo "Executing target '$@' for project '$(PRJ_NAME)'"
	@$(LINK.time) $(CXX) --sysroot="$(SYSTEM_ROOT_PATH)" $(OBJECT_FILES) $(SANITIZERS) $(LD_FLAGS) -o $(EXECUTABLE)
	@echo "Finished executing the target '$@' for project '$(PRJ_NAME)'"

clean:
	@echo "Executing target '$@' for project '$(PRJ_NAME)'"
	rm -rf $(PRJ_OUTPUT_DIR)/*
	@echo "Finished cleaning the target '$@' for project '$(PRJ_NAME)'"

# Generic C/C++ files build target (Object generation)
$(PRJ_OBJECTS_DIR)/%.o : %.cc
$(PRJ_OBJECTS_DIR)/%.o : %.cc $(DEPDIR)/%.d | $(DEPDIR)
	@echo "Building '$<' for project '$(PRJ_NAME)'"
	@mkdir -p $(@D)
	$(COMPILE.cpp) $< -o $@
	@echo "Built '$<' for project '$(PRJ_NAME)'"

$(PRJ_OBJECTS_DIR)/%.o : %.cpp
$(PRJ_OBJECTS_DIR)/%.o : %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	@echo "Building '$<' for project '$(PRJ_NAME)'"
	@mkdir -p $(@D)
	@$(COMPILE.cpp) $< -o $@
	@echo "Built '$<' for project '$(PRJ_NAME)'"

# Generic C/C++ files build target (Object generation)
$(PRJ_OBJECTS_DIR)/%.o : %.c
$(PRJ_OBJECTS_DIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	@echo "Building '$<' for project '$(PRJ_NAME)'"
	@mkdir -p $(@D)
	@$(COMPILE.c) $< -o $@
	@echo "Built '$<' for project '$(PRJ_NAME)'"

$(DEPDIR): ; @mkdir -p $@
$(DEP_FILES):
include $(wildcard $(DEP_FILES))

# Prevent confusion between files and targets
.PHONY: all clean config_step
