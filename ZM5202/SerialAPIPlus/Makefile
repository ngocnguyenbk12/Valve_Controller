#****************************************************************************
#
#  Copyright (c) 2001-2014
#  Sigma Designs, Inc.
#  All Rights Reserved
#
#----------------------------------------------------------------------------
#
# Description:
#    Makefile for the ZW0x0x development kit SerialAPIPlus
#    This makefile defines the targets built
#
# Author:   Erik Friis Harck
#
# Last Changed By:  $Author: efh $
# Revision:         $Revision: 29880 $
# Last Changed:     $Date: 2014-11-10 13:17:38 +0100 (ma, 10 nov 2014) $
#
#****************************************************************************

#---------------------------------------------------------------------------------------------------------
# HOW-TO: Specifying which subset of "everything", you wan't to build:
#
# There is a list of build configuration parameters to be set (variants).
# If one is not set (commented out), the application will be built for all possible instances from a list,
# else the single one set will be built.
# You can specify a single instance to build for in 3 different ways:
# 1. By specifying the instance in your command line, like:
#    > make FREQUENCY=EU ....
# 2. By setting the instance just below like
#    FREQUENCY:=EU
# 3. Alternativly you can do the same by setting your environment with:
#    > SET FREQUENCY=EU
#    Remember to UNSET this when you jump to work on other things.
# And yes... You can combine these methods in any way.
# You can always use help to see the possible combinations, like
#    > make help
#
#LIBRARY:=controller_static
#FREQUENCY:=EU
CODE_MEMORY_MODE:=banking
SENSOR_TYPE:=NON_BATT
CHIP:=ZW050x
WATCHDOG:=WATCHDOG_DISABLED
#BOOT:=nonBOOT

#---------------------------------------------------------------------------------------------------------

# This is the default target file name end (chip | module | board or whatever you like)
CHIPPACK:=ZW050x

#---------------------------------------------------------------------------------------------------------

# Full list of all supported libraries can be found in $(ZWLIBROOT)\Common\Makefile.common
# Downgraded list of supported libraries for SerialAPIPlus.
LIST_OF_LIBRARY:=\
 controller_bridge\
 controller_portable\
 controller_static\
 controller_static_single\
 slave_enhanced_232\
 slave_routing

#---------------------------------------------------------------------------------------------------------
# If not supplied by application here, the name of the application will be set equal to it's directory name.
APP_NAME:=serialapi

ifneq ($(BOOTLOADER_ENABLED),)
ifeq ($(LIBRARY),slave_routing)
$(warning slave_routing do not support BOOTLOADER_ENABLED target)
endif

#ifeq ($(LIBRARY),slave_routing)
#LIBRARY:=slave_enhanced_232
#override LIB:=slave_enhanced_232
#endif
#LIST_OF_LIBRARY:=$(filter-out slave_routing,$(LIST_OF_LIBRARY))
#ifeq ($(LIBRARY),slave_routing)
#BOOTLOADER_ENABLED:=
#endif
endif

ifeq ($(LIBRARY),slave_routing)
LIST_OF_BOOT:=nonBOOT
BOOT:=nonBOOT
BOOTLOADER_ENABLED:=

$(info slave_routing do not support BOOTLOADER - filtered out)
endif

#---------------------------------------------------------------------------------------------------------
# List of all application modules.

RELFILES:=\
 serialappl.obj\
 conhandle.obj\
 led_control.obj\
 ZW_nvr_app_api.obj\
 eeprom.obj\
 nvmHost.obj

#---------------------------------------------------------------------------------------------------------
# Library specific modules.

#ifneq ($(findstring controller, $(LIBRARY)),)
#ifeq ($(findstring single, $(LIBRARY)),)
#RELFILES+=\
#  ctrl_learn.obj
#endif
#endif

ifeq ($(LIBRARY),slave_enhanced_232)
# For now only slave_enhanced_232 targets can use the slave_learn_plus module
#RELFILES+=\
#  slave_learn_plus.obj

# Z-Wave libraries containing Secutrity S0/S2 functionality needs these modules
RELFILES+=\
 ZW_cmd_class_list.obj\
 ZW_TransportSecProtocol.obj

endif

ifeq ($(LIBRARY),slave_routing)
# For now only slave_routing targets can use the slave_learn_plus module
#RELFILES+=\
#  slave_learn_plus.obj

# Z-Wave libraries containing Secutrity S0/S2 functionality needs these modules
RELFILES+=\
 ZW_cmd_class_list.obj\
 ZW_TransportSecProtocol.obj

endif

ifeq ($(LIBRARY),slave_routing)
## Do not include ZW_nvm_descriptor.obj if library=slave_routing
else
RELFILES+=\
 ZW_nvm_descriptor.obj
endif


# The two following make variables, APPLICATION_NVMLOC and NVMCONSTLOC, must have a matching sequence.

# Set APPLICATION_NVMLOC to make sure the order of nvmModules in HDATA space will be correct:
# ?FD?ZW_NVM            ZW_library NVM variables                                MUST be first (lowest address)
# ?FD?ZW_NVM_S2		ZW_library Security NVM variables 			- ONLY include IF SECURITY_INCLUDED=yes
# ?FD?EEPROM            Your application NVM variables
# ?FD?NVMHOST           Your host application NVM variables
# ?FD?ZW_NVM_DESCRIPTOR t_nvmDescriptor far nvmDescriptor                       MUST be last (highest address)
# (do not include ?FD?ZW_NVM in APPLICATION_NVMLOC. This is located at fixed address 0 in HDATA because of external dependencies)
ifeq ($(LIBRARY),slave_enhanced_232)
APPLICATION_NVMLOC:=?FD?ZW_NVM_S2,?FD?EEPROM,?FD?NVMHOST
else
ifeq ($(LIBRARY),slave_routing)
APPLICATION_NVMLOC:=?FD?ZW_NVM_S2,?FD?EEPROM,?FD?NVMHOST
else
APPLICATION_NVMLOC:=?FD?EEPROM,?FD?NVMHOST
endif
endif

ifneq ($(LIBRARY),slave_routing)
# Only include ZW_NVM_DESCRIPTOR if NOT slave_routing based
APPLICATION_NVMLOC+=,?FD?ZW_NVM_DESCRIPTOR
endif

ifneq ($(LIBRARY),slave_routing)
# Set NVMCONSTLOC to make sure the order of nvmDescriptorUpdates in CODE space will be correct:
# ?CO?ZW_NVM (const t_nvmModuleUpdate code nvmZWlibraryUpdate)                  MUST be first (lowest address)
# ?CO?ZW_NVM_S2	(const t_nvmModuleUpdate code nvmSecurity2Update) 		- ONLY include IF SECURITY_INCLUDED=yes
# ?CO?EEPROM (const t_nvmModuleUpdate code nvmApplicationUpdate)
# ?CO?NVMHOST (const t_nvmModuleUpdate code nvmHostApplicationUpdate)
# ?CO?ZW_NVM_DESCRIPTOR (const t_nvmModuleUpdate code nvmDescriptorUpdate)      MUST be last (highest address)
ifeq ($(LIBRARY),slave_enhanced_232)
NVMCONSTLOC:=?CO?ZW_NVM,?CO?ZW_NVM_S2,?CO?EEPROM,?CO?NVMHOST,?CO?ZW_NVM_DESCRIPTOR
else
NVMCONSTLOC:=?CO?ZW_NVM,?CO?EEPROM,?CO?NVMHOST,?CO?ZW_NVM_DESCRIPTOR
endif

endif

#---------------------------------------------------------------------------------------------------------

# Include production targets (if any)
-include Makefile.ProductionTargets

# Include HostInterfaceTargets variants (HOST_INTERFACE=...) (before calling the recursive engine)
include Makefile.HostInterfaceTargets

#---------------------------------------------------------------------------------------------------------
# Target name (Name of your target directory and base name of your target files)

TARGET:=$(APP)_$(LIB)
ifneq ($(BOOTLOADER_ENABLED),)
TARGET:=$(TARGET)_OTW
endif
TARGETDIR:=$(TARGET)_$(CHIPPACK)$(IFNAME)$(TEST)
TARGET:=$(TARGET)_$(CHIPPACK)$(IFNAME)_$(COUNTRY)$(TEST)

#---------------------------------------------------------------------------------------------------------
# Compute and setup references to Z-Wave library and utils (framework or whatever)
#
# Don't change this. If you want to reference another Z-Wave library or utils, use these
# environment settings instead:
#
# SET ZWLIBROOT=<full path of your alternative Z-Wave library location>
# SET ZWUTILDIR=<full path of your alternative ZWUTILDIR framework location>

# Find the current directory
# used as a base for all other paths.
BASEDIR=$(shell cd)

# Find the product name from the subdirectoryname
BASEDIRWORDLIST=$(subst \, ,$(BASEDIR))
BASEDIRLASTWORDINDEX=$(words $(BASEDIRWORDLIST))
PRODUCTDIR=$(word $(BASEDIRLASTWORDINDEX),$(BASEDIRWORDLIST))

# Find the Z-Wave library location (if not forced by setting the environment variable ZWLIBROOT)
ifndef ZWLIBROOT
ZWLIBROOT:=$(subst \$(PRODUCTDIR),,$(subst \ProductPlus,\Z-Wave,$(BASEDIR)))
endif

# Library location when making release of application
ifdef SVNLIBRELEASEDIRBIN
ZWLIBROOT:=$(SVNLIBRELEASEDIRBIN)
endif

# Export the variables declared above to the other makefiles.
export BASEDIR ZWLIBROOT UNDERSTAND_C

# Utility location
ifeq ($(ZWUTILDIR),)
ZWUTILDIR:=..\ApplicationUtilities
endif

#---------------------------------------------------------------------------------------------------------
# After setting up the build environment include some common makefiles, which defines how to build.

include $(ZWUTILDIR)\Makefile.common

# Include HostInterfaceTargets variants (HOST_INTERFACE=...) (after having called the recursive engine)
include Makefile.HostInterfaceTargets

# Filter out unsupported frequencies for the Z-Wave application.
# Full list of all supported frequencies can be found in $(ZWUTILDIR)\Makefile.common
# (this filter-out must be done after/below the inclusion of $(ZWUTILDIR)\Makefile.common)
LIST_OF_FREQUENCY:=$(filter-out ALL ALL_3CH,$(LIST_OF_FREQUENCY))

#---------------------------------------------------------------------------------------------------------
# ADDITION TO THE VARIANTS TO BUILD
#---------------------------------------------------------------------------------------------------------
# Add a dimension "BOOT" to the build array
#
# BOOT variant must be added in front of LIST_OF_VARIANTS to be handled last,
# and for the filtering below in this Makefile to work. (LIST_OF_VARIANTS+= BOOT # does not work correctly)
LIST_OF_VARIANTS:=BOOT $(LIST_OF_VARIANTS)
# A full list of values for each VariantName must be assigned.
LIST_OF_BOOT:=BOOTLOADER_ENABLED nonBOOT
# A default value for each VariantName must be assigned. They are used in the help texts.
BOOT_DEFAULT:=nonBOOT
# An option variable for each VariantName must be assigned to be considered during "make",
# and of course, the application source code must be prepared to distinguish between variants.
ifeq ($(BOOT),BOOTLOADER_ENABLED)
BOOT_OPTION:=BOOTLOADER_ENABLED=1
BOOTLOADER_ENABLED:=1
endif
#---------------------------------------------------------------------------------------------------------


#---------------------------------------------------------------------------------------------------------
# Filtering out targets, which have no meaning, or are just not needed.
#
# The controlling variant name must be listed before the controlled variant name in the LIST_OF_VARIANTS
# for this to work.
#
ifeq ($(BOARD),ZM5304)
# Only controller_static makes sense for ZM5304 module
LIST_OF_LIBRARY:=controller_bridge controller_static controller_static_single
endif
#---------------------------------------------------------------------------------------------------------


#---------------------------------------------------------------------------------------------------------
# Filtering out targets, which have no meaning, or are just not needed.
#
# The controlling variant name must be listed before the controlled variant name in the LIST_OF_VARIANTS
# for this to work.
#
ifneq ($(BOOTLOADER_ENABLED),)
LIST_OF_LIBRARY:=\
 controller_bridge\
 controller_portable\
 controller_static\
 slave_enhanced_232
endif
#---------------------------------------------------------------------------------------------------------

ifneq ($(UVISIONPREBUILD),) # If part of uVision build, do not make SupportedFunction txt files for SerialAPI targets
else
-include Makefile.common_ZW0x0x_supported_functions
endif

# Generate SerialAPIPlus defines for supported functions based on what exists in library.
lib_supported_func.h: $(LIB)_supported_func.h
	@echo #include "$(LIB)_supported_func.h" > $(subst \,\\,$(RELSDIRFULL)\lib_supported_func.h)
$(LIB)_supported_func.h: $(LIBFILE) serialapi-supported-func-list.txt
	make-supported-functions-include.bat $(subst \,\\,$(LIBFILE)) $(subst \,\\,$(TARGETBASEDIRFULL)\$(LIB)_supported_func.h)

# Make sure lib_supported_func.h is generated before we generate dependencies
ifneq ($(UVISIONPREBUILD),) # If part of uVision build, generate lib_supported_func.h witout generating dependencies. uVision has it's own way to handle this.
depend: lib_supported_func.h
else
Makefile.d: lib_supported_func.h
endif

ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 ZW_ID_SERVER,\
 SUC_SUPPORT,\
 SERIAL_API_APP

## Missing C defines common for all libraries, except controller single, should be defined below
ifneq ($(LIBRARY),controller_static_single)
ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 ZW_SMARTSTART_ENABLED,\
 ZW_EXPLORE
endif

ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 ENABLE_LEDS
 
ifneq ($(CDEFINES),)
CDEFINES+=,
endif
CDEFINES+=\
 $(SENSOR_TYPE)
