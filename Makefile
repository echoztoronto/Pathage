# Copyright 2018 University of Toronto
# 
# Permission is hereby granted, to use this software and associated 
# documentation files (the "Software") in course work at the University 
# of Toronto, or for personal use. Other uses are prohibited, in 
# particular the distribution of the Software either publicly or to third 
# parties.
# 
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


################################################################################
#
# NOTE: It is always *your* responsibility to ensure your code works
#       with the autotester and can be submitted successfully.
#
#       We can not grade code that fails to compile/link.
#
#       After any changes verify that your code builds and submits successfully!
################################################################################

#If you wish to add custom linking/compile flags add them to the
# CUSTOM_LINK_FLAGS and CUSTOM_COMIPLE_FLAGS. The autotester will
# use these determine how to compile/link your code.
#
# For example, setting:
#
#	CUSTOM_COMPILE_FLAGS = -fopenmp
#	CUSTOM_LINK_FLAGS = -fopenmp
#
# will enable OpenMP (the fopenmp flag needs to be specified both during
# compilation and linking),
#
# While setting:
#
#	CUSTOM_LINK_FLAGS = -lreadline
#
# will cause the autotester to link to the GNU Readline library.
#
# Multiple options can be combined to enable multiple libraries/features

#
# By default the following custom flags are enabled:
#    * pkg-config is used to set all compile and linking flags required by EasyGL (x11, xft, fontconfig, cairo)
#    * -fopenmp (OpenMP for parallel programming)
#    * -lreadline (interactive line editing library)
#
CUSTOM_COMPILE_FLAGS = $(shell pkg-config --cflags x11 xft fontconfig cairo) -fopenmp 
CUSTOM_LINK_FLAGS    = $(shell pkg-config --libs x11 xft fontconfig cairo) -fopenmp -lreadline 

################################################################################
#	                ! WARNING - Here Be Dragons - WARNING !
#
#               Editting below this point should not be neccessary,
#                         and may break the autotester.
#
#                    Make sure you know what you are doing!
################################################################################

################################################################################
# Utility Functions
################################################################################

#Recursive version of wildcard (i.e. it checks all subdirectories)
# For example:
# 	$(call rwildcard, /tmp/, *.c *.h)
# will return all files ending in .c or .h in /tmp and any of tmp's sub-directories
#NOTE: the directory (e.g. /tmp/) should end with a slash (i.e. /)
rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

#Recursively find directories containing specifc files (useful for finding header include directories)
# For example:
# 	$(call rfiledirs, /tmp/, *.h *.hpp)
# will return all directories in /tmp which contain .h or .hpp files
#NOTE: the directory (e.g. /tmp/) should end with a slash (i.e. /)
rfiledirs=$(sort $(dir $(call rwildcard, $1, $2)))

################################################################################
# Configuration
################################################################################
#Default build type
CONF=release

$(info Configuration is '$(CONF)')

#
# Tool names
#

#The name of the compiler to use
CXX = g++

#Name of the archiver used to create .a from sets of .o files
AR = ar

#
# Directories
#

#Directory for build temporaries
BUILD_DIR = build

#What directory contains the source files for the main executable?
EXE_SRC_DIR = main/src

#What directory contains the source files for the street map static library?
LIB_STREETMAP_SRC_DIR = libstreetmap/src/

#What directory contains the source files for the street map library tests?
LIB_STREETMAP_TEST_DIR = libstreetmap/tests/

#Global directory to look for custom library builds
ECE297_ROOT ?= /cad2/ece297s/public
ECE297_LIB_DIR ?= $(ECE297_ROOT)/lib
ECE297_INCLUDE_DIR ?= $(ECE297_ROOT)/include

#
#Output files
#

#Name of the primary executable
EXE=mapper

#Name of the test executable
LIB_STREETMAP_TEST=test_libstreetmap

#Name of the street map static library
LIB_STREETMAP=libstreetmap.a

################################################################################
# External Library Configuration
################################################################################
ECE297_MILESTONE_INCLUDE_DIRS ?= $(ECE297_INCLUDE_DIR)/milestones

#Boost serialization
BOOST_SERIALIZATION_RELEASE_LIB := -lboost_serialization
BOOST_SERIALIZATION_DEBUG_CHECK_LIB := $(ECE297_LIB_DIR)/debug_check/libboost_serialization.a
BOOST_SERIALIZATION_LIB := $(BOOST_SERIALIZATION_RELEASE_LIB) #Defines the version actually used, default release

#Streets database
STREETS_DATABASE_RELEASE_LIB := $(ECE297_LIB_DIR)/release/libstreetsdatabase.a
STREETS_DATABASE_DEBUG_CHECK_LIB := $(ECE297_LIB_DIR)/debug_check/libstreetsdatabase.a
STREETS_DATABASE_LIB := $(STREETS_DATABASE_RELEASE_LIB) #Defines the version actually used, default release
STREETS_DATABASE_INCLUDE_DIRS := $(ECE297_INCLUDE_DIR)/streetsdatabase

#Unittest++
UNITTESTPP_RELEASE_LIB := -lunittest++
UNITTESTPP_DEBUG_CHECK_LIB := $(ECE297_LIB_DIR)/debug_check/libunittest++.a
UNITTESTPP_LIB := $(UNITTESTPP_RELEASE_LIB) #Defines the version actually used, default release

#Choose the correct external library builds.
#If we are using a debug build AND we have a separately compiled debug 
#build of a library use it instead of the system default
ifeq (debug_check, $(findstring debug_check,$(CONF))) #Doing a debug check build?

#Boost serialization
ifneq (, $(wildcard $(BOOST_SERIALIZATION_DEBUG_CHECK_LIB))) 
	#Debug lib file exists, use it
BOOST_SERIALIZATION_LIB := $(BOOST_SERIALIZATION_DEBUG_CHECK_LIB)
endif

#Streets database
ifneq (, $(wildcard $(STREETS_DATABASE_DEBUG_CHECK_LIB))) 
	#Debug lib file exists, use it
STREETS_DATABASE_LIB := $(STREETS_DATABASE_DEBUG_CHECK_LIB)
endif

#Unittest++
ifneq (, $(wildcard $(UNITTESTPP_DEBUG_CHECK_LIB))) #Debug lib file exits
	#Debug lib file exists, use it
UNITTESTPP_LIB := $(UNITTESTPP_DEBUG_CHECK_LIB)
endif

endif #debug

################################################################################
# Tool flags
################################################################################

#What warning flags should be passed to the compiler?
WARN_CFLAGS = -Wall \
			 -Wextra \
			 -Wpedantic \
			 -Wcast-qual \
			 -Wcast-align \
			 -Wformat=2 \
			 -Wlogical-op \
			 -Wmissing-declarations \
			 -Wmissing-include-dirs \
			 -Wswitch-default \
			 -Wundef \
			 -Wunused-variable \
			 -Woverloaded-virtual \
			 -Wctor-dtor-privacy \
			 -Wnon-virtual-dtor \
			 -Wold-style-cast \
			 -Wshadow \
			 -Wredundant-decls \
			 #-Wconversion \
			 #-Wno-sign-conversion

# Find all the include directoires for each component
LIB_STREETMAP_INCLUDE_FLAGS = $(foreach dir, $(call rfiledirs, $(LIB_STREETMAP_SRC_DIR), *.h *.hpp), -I$(dir))
EXE_INCLUDE_FLAGS = $(foreach dir, $(call rfiledirs, $(EXE_SRC_DIR), *.h *.hpp), -I$(dir))
STREETS_DATABASE_INCLUDE_FLAGS = $(foreach dir, $(STREETS_DATABASE_INCLUDE_DIRS), -I$(dir))
ECE297_MILESTONE_INCLUDE_FLAGS = $(foreach dir, $(ECE297_MILESTONE_INCLUDE_DIRS), -I$(dir))

#What include flags should be passed to the compiler?
INCLUDE_CFLAGS = $(LIB_STREETMAP_INCLUDE_FLAGS) $(ECE297_MILESTONE_INCLUDE_FLAGS) $(STREETS_DATABASE_INCLUDE_FLAGS) $(EXE_INCLUDE_FLAGS)

#What options to generate header dependency files should be passed to the compiler?
DEP_CFLAGS = -MMD -MP

#What extra flags to use in a debug build?
DEBUG_CFLAGS = -g -O0
DEBUG_LFLAGS =

# Defining _GLIBCXX_DEBUG enables extra STL checking (e.g. vector [] bounds checking, iterator validity)
# Specifying -fsanitize enabled run-time sanitizer checking (e.g. bad memory access, leaks, undefined behaviour)
DEBUG_CHECK_CFLAGS = -g -O0 -D_GLIBCXX_DEBUG -fsanitize=address -fsanitize=leak -fsanitize=undefined
DEBUG_CHECK_LFLAGS = -fsanitize=address -fsanitize=leak -fsanitize=undefined

#What extra flags to use in a release build?
RELEASE_CFLAGS = -g -O3
RELEASE_LFLAGS =

#What extra flags to use in release profile build?
# Note: provide -g so symbols are included in profiling info
PROFILE_CFLAGS = -g -O3 -pg -fno-inline
PROFILE_LFLAGS = -pg

#Pick either debug/release/profile build flags 
ifeq (release, $(CONF))
CONF_CFLAGS := $(RELEASE_CFLAGS)
CONF_LFLAGS := $(RELEASE_LFLAGS)
else ifeq (debug, $(CONF))
CONF_CFLAGS := $(DEBUG_CFLAGS)
CONF_LFLAGS := $(DEBUG_LFLAGS)
else ifeq (debug_check, $(CONF))
CONF_CFLAGS := $(DEBUG_CHECK_CFLAGS)
CONF_LFLAGS := $(DEBUG_CHECK_LFLAGS)
else ifeq (profile, $(CONF))
CONF_CFLAGS := $(PROFILE_CFLAGS)
CONF_LFLAGS := $(PROFILE_LFLAGS)
else
$(error Invalid value for CONF: '$(CONF)', must be 'release', 'debug', 'debug_check', 'profile'. Try 'make help' for usage)
endif

#Collect all the options to give to the compiler
CFLAGS = --std=c++14 $(CONF_CFLAGS) $(DEP_CFLAGS) $(WARN_CFLAGS) $(INCLUDE_CFLAGS) $(CUSTOM_COMPILE_FLAGS)

#Flags for linking
LFLAGS = $(CONF_LFLAGS) -L. $(STREETS_DATABASE_LIB) $(BOOST_SERIALIZATION_LIB) $(GRAPHICS_LFLAGS) $(CUSTOM_LINK_FLAGS)

#
# Archiver flags
#

#Flags for the archiver (used to create static libraries)
ARFLAGS = rvs

################################################################################
# Generate object file names from source file names
################################################################################

#Objects associated with the main executable
EXE_OBJ=$(patsubst %.cpp, $(BUILD_DIR)/$(CONF)/%.o, $(call rwildcard, $(EXE_SRC_DIR), *.cpp))

#Objects associated with the street map library
LIB_STREETMAP_OBJ=$(patsubst %.cpp, $(BUILD_DIR)/$(CONF)/%.o, $(call rwildcard, $(LIB_STREETMAP_SRC_DIR), *.cpp))

#Objects associated with tests for the street map library
# We collect tests both from the local project (under LIB_STREETMAP_TEST_DIR)
LIB_STREETMAP_TEST_OBJ=$(patsubst %.cpp, $(BUILD_DIR)/$(CONF)/%.o, \
					   	$(call rwildcard, $(LIB_STREETMAP_TEST_DIR), *.cpp) \
					   )

################################################################################
# Dependency files
################################################################################

#To capture dependencies on header files,
# we told the compiler to generate dependency 
# files associated with each object file
#
#The ':.o=.d' syntax means replace each filename ending in .o with .d
# For example:
#   build/main/main.o would become build/main/main.d
DEP = $(EXE_OBJ:.o=.d) $(LIB_STREETMAP_OBJ:.o=.d) $(LIB_STREETMAP_TEST_OBJ:.o=.d)

################################################################################
# Make targets
################################################################################

#Phony targets are always run (i.e. are always out of date)
#  We mark all executables and static libraries as phony so that they will
#  always be re-linked. This ensures that all final libraries/executables 
#  will be of the same build CONF. This is important since using _GLIBCXX_DEBUG 
#  can cause the debug and release builds to be binary incompatible, causing odd 
#  errors if both debug and release components are mixed.
.PHONY: clean $(EXE) $(LIB_STREETMAP_TEST) $(LIB_STREETMAP)

#The default target
# This is called when you type 'make' on the command line
all: $(EXE)

#This runs the unit test executable
test: $(LIB_STREETMAP_TEST)
	@echo ""
	@echo "Running Unit Tests..."
	./$(LIB_STREETMAP_TEST)

#Include header file dependencies generated by a
# previous compile
-include $(DEP)

#Link main executable
$(EXE): $(EXE_OBJ) $(LIB_STREETMAP)
	$(CXX) $^ $(LFLAGS) -o $@

#Link test executable
$(LIB_STREETMAP_TEST): $(LIB_STREETMAP_TEST_OBJ) $(LIB_STREETMAP)
	$(CXX) $^ $(UNITTESTPP_LIB) $(LFLAGS) -o $@

#Street Map static library
$(LIB_STREETMAP): $(LIB_STREETMAP_OBJ)
	$(AR) $(ARFLAGS) $@ $^

#Note: % matches recursively between prefix and suffix
#      so %.cpp would match both src/a/a.cpp
#      and src/b/b.cpp
$(BUILD_DIR)/$(CONF)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*
	rm -f $(EXE) $(LIB_STREETMAP) $(LIB_STREETMAP_TEST)

custom_flags:
	@echo "CUSTOM_COMPILE_FLAGS: $(CUSTOM_COMPILE_FLAGS)"
	@echo "CUSTOM_LINK_FLAGS: $(CUSTOM_LINK_FLAGS)"

echo_flags:
	@echo "COMPILE_FLAGS: $(CFLAGS)"
	@echo "LINK_FLAGS: $(LFLAGS)"

help:
	@echo "Makefile for ECE297"
	@echo ""
	@echo "Usage: "
	@echo '    > make'
	@echo "        Call the default make target (all)."
	@echo "        This builds the project executable: '$(EXE)'."
	@echo "    > make clean"
	@echo "        Removes any generated files including exectuables,"
	@echo "        static libraries, and object files."
	@echo "    > make test"
	@echo "        Runs unit tests."
	@echo "        Builds and runs any tests found in $(LIB_STREETMAP_TEST_DIR),"
	@echo "        generating the test executable '$(LIB_STREETMAP_TEST)'."
	@echo "    > make custom_flags"
	@echo "        Echos the custom compile and link flags."
	@echo "		   This is used by the autotester to figure out how compile and link your code."
	@echo "    > make echo_flags"
	@echo "        Echos the compile and link flags used by the make file."
	@echo "    > make help"
	@echo "        Prints this help message."
	@echo ""
	@echo ""
	@echo "Configuration Variables: "
	@echo "    CONF={release | debug | debug_check | profile}"
	@echo "        Controls whether the build performs compiler optimizations"
	@echo "        to improve performance. Currently set to '$(CONF)'."
	@echo ""
	@echo "        With CONF=release compiler optimization is enabled ($(RELEASE_CFLAGS))."
	@echo ""
	@echo "        With CONF=debug compiler optimization is disabled to improve"
	@echo "        interactive debugging ($(DEBUG_CFLAGS))."
	@echo ""
	@echo "        With CONF=debug_check compiler optimization is disabled, and extra"
	@echo "        error checking enabled in the standard library, invalid memory access"
	@echo "        and undefined behaviour ($(DEBUG_CHECK_CFLAGS))."
	@echo ""
	@echo "        With CONF=profile a release-like build with gprof profiling "
	@echo "        enabled ($(PROFILE_CFLAGS))."
	@echo ""
	@echo "        You can configure specify this option on the command line."
	@echo "        To perform a release build you can use: "
	@echo "            > make CONF=release"
	@echo "        To perform a regular debug build you can use: "
	@echo "            > make CONF=debug"
	@echo "        To perform a debug build with extra checking (runs slower) you can use: "
	@echo "            > make CONF=debug_check"
	@echo "        To perform a profile build you can use: "
	@echo "            > make CONF=profile"
