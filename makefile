#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# The top part of this file contains all the application-specific config
# settings.  Everything beyond that is generic and will be the same for
# every application you use this makefile template for.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#-----------------------------------------------------------------------------
# This is the base name of the executable file
#-----------------------------------------------------------------------------
EXE = radlogger


#-----------------------------------------------------------------------------
# This is a list of directories that have compilable code in them.  If there
# are no subdirectories, this line is must SUBDIRS = .
#-----------------------------------------------------------------------------
SUBDIRS = . 


#-----------------------------------------------------------------------------
# For x86, declare whether to emit 32-bit or 64-bit code
#-----------------------------------------------------------------------------
X86_TYPE = 64


#-----------------------------------------------------------------------------
# These are the language standards we want to compile with
#-----------------------------------------------------------------------------
C_STD = -std=gnu99
CPP_STD = -std=c++17


#-----------------------------------------------------------------------------
# Declare the compile-time flags that are common between all platforms
#-----------------------------------------------------------------------------
CXXFLAGS =	\
-O2 -g -Wall \
-c -fmessage-length=0 \
-D_GNU_SOURCE \
-Wno-sign-compare \
-Wno-unused-value 

#-----------------------------------------------------------------------------
# Link options
#-----------------------------------------------------------------------------
LINK_FLAGS = -pthread -lm -lrt


#-----------------------------------------------------------------------------
# If there is no target on the command line, this is the target we use
#-----------------------------------------------------------------------------
.DEFAULT_GOAL := x86

#-----------------------------------------------------------------------------
# Define the name of the compiler and what "build all" means for our platform
#-----------------------------------------------------------------------------
ALL       = x86 arm
X86_CC    = $(CC)
X86_CXX   = $(CXX)
X86_STRIP = strip

ARM_PATH  = /bin/aarch64-linux-gnu
ARM_CC    = $(ARM_PATH)-gcc
ARM_CXX   = $(ARM_PATH)-g++
ARM_STRIP = $(ARM_PATH)-strip

#-----------------------------------------------------------------------------
# Declare where the object files get created
#-----------------------------------------------------------------------------
X86_OBJ_DIR := obj_x86
ARM_OBJ_DIR := obj_arm


#-----------------------------------------------------------------------------
# Always run the recipe to make the following targets
#-----------------------------------------------------------------------------
.PHONY: $(X86_OBJ_DIR) $(ARM_OBJ_DIR)


#-----------------------------------------------------------------------------
# We're going to compile every .c and .cpp file in each directory
#-----------------------------------------------------------------------------
C_SRC_FILES   := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
CPP_SRC_FILES := $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.cpp))


#-----------------------------------------------------------------------------
# In the source files, normalize "./filename" to just "filename"
#-----------------------------------------------------------------------------
C_SRC_FILES   := $(subst ./,,$(C_SRC_FILES))
CPP_SRC_FILES := $(subst ./,,$(CPP_SRC_FILES))


#-----------------------------------------------------------------------------
# Create the base-names of the object files
#-----------------------------------------------------------------------------
C_OBJ     := $(C_SRC_FILES:.c=.o)
CPP_OBJ   := $(CPP_SRC_FILES:.cpp=.o)
OBJ_FILES := ${C_OBJ} ${CPP_OBJ}


#-----------------------------------------------------------------------------
# We are going to keep x86 and ARM object files in separate sub-directories
#-----------------------------------------------------------------------------
X86_OBJS := $(addprefix $(X86_OBJ_DIR)/,$(OBJ_FILES))
ARM_OBJS := $(addprefix $(ARM_OBJ_DIR)/,$(OBJ_FILES))

#-----------------------------------------------------------------------------
# This rules tells how to compile an X86 .o object file from a .cpp source
#-----------------------------------------------------------------------------
$(X86_OBJ_DIR)/%.o : %.cpp
	$(X86_CXX) -m$(X86_TYPE) $(CPPFLAGS) $(CPP_STD) $(CXXFLAGS) -c $< -o $@

$(X86_OBJ_DIR)/%.o : %.c
	$(X86_CC) -m$(X86_TYPE) $(CPPFLAGS) $(C_STD) $(CXXFLAGS) -c $< -o $@


#-----------------------------------------------------------------------------
# This rules tells how to compile an ARM .o object file from a .cpp source
#-----------------------------------------------------------------------------
$(ARM_OBJ_DIR)/%.o : %.cpp
	$(ARM_CXX) $(CPPFLAGS) $(CPP_STD) $(CXXFLAGS) $(ARMFLAGS) -c $< -o $@

$(ARM_OBJ_DIR)/%.o : %.c
	$(ARM_CC) $(CPPFLAGS) $(C_STD) $(CXXFLAGS) $(ARMFLAGS) -c $< -o $@


#-----------------------------------------------------------------------------
# This rule builds the x86 executable from the object files
#-----------------------------------------------------------------------------
$(EXE).x86 : $(X86_OBJS)
	$(X86_CXX) -m$(X86_TYPE) -o $@ $(X86_OBJS) $(LINK_FLAGS)
	$(X86_STRIP) $(EXE).x86

#-----------------------------------------------------------------------------
# This rule builds the x86 executable from the object files
#-----------------------------------------------------------------------------
$(EXE).arm : $(ARM_OBJS)
	$(ARM_CXX) -o $@ $(ARM_OBJS) $(LINK_FLAGS)
	$(ARM_STRIP) $(EXE).arm

#-----------------------------------------------------------------------------
# This target builds all executables supported by this platform
#-----------------------------------------------------------------------------
all:	$(ALL)


#-----------------------------------------------------------------------------
# This target builds just the x86 executable
#-----------------------------------------------------------------------------
x86:	$(X86_OBJ_DIR) $(EXE).x86


#-----------------------------------------------------------------------------
# This target builds just the ARM executable
#-----------------------------------------------------------------------------
arm:	$(ARM_OBJ_DIR) $(EXE).arm


#-----------------------------------------------------------------------------
# These targets makes all neccessary folders for object files
#-----------------------------------------------------------------------------
$(X86_OBJ_DIR):
	@for subdir in $(SUBDIRS); do \
	    mkdir -p -m 777 $(X86_OBJ_DIR)/$$subdir ;\
	done

$(ARM_OBJ_DIR):
	@for subdir in $(SUBDIRS); do \
	    mkdir -p -m 777 $(ARM_OBJ_DIR)/$$subdir ;\
	done

#-----------------------------------------------------------------------------
# This target removes all files that are created at build time
#-----------------------------------------------------------------------------
clean:
	rm -rf Makefile.bak makefile.bak $(EXE).tgz 
	rm -rf $(X86_OBJ_DIR) $(EXE).x86
	rm -rf $(ARM_OBJ_DIR) $(EXE).arm



#-----------------------------------------------------------------------------
# This target creates a compressed tarball of the source code
#-----------------------------------------------------------------------------
tarball:	clean
	rm -rf $(EXE).tgz
	tar --create --exclude-vcs -v -z -f $(EXE).tgz *


#-----------------------------------------------------------------------------
# This target appends/updates the dependencies list at the end of this file
#-----------------------------------------------------------------------------
depend:
	@makedepend    -p$(X86_OBJ_DIR)/ $(C_SRC_FILES) $(CPP_SRC_FILES) -Y 2>/dev/null


#-----------------------------------------------------------------------------
# Convenience target for displaying makefile variables 
#-----------------------------------------------------------------------------
debug:
	@echo "SUBDIRS       = ${SUBDIRS}"
	@echo "C_SRC_FILES   = ${C_SRC_FILES}"
	@echo "CPP_SRC_FILES = ${CPP_SRC_FILES}"
	@echo "C_OBJ         = ${C_OBJ}"
	@echo "CPP_OBJ       = ${CPP_OBJ}"
	@echo "OBJ_FILES     = ${OBJ_FILES}"


#-----------------------------------------------------------------------------




# DO NOT DELETE

obj_x86/access_lib/gpioAccess.o: access_lib/llnseConnImpl.h
obj_x86/access_lib/gpioAccess.o: access_lib/llnse_messages.h
obj_x86/access_lib/llnseConn.o: access_lib/llnseConnImpl.h
obj_x86/access_lib/llnseConn.o: access_lib/llnse_messages.h
