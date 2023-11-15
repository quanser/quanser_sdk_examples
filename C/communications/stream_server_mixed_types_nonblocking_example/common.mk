# This is an automatically generated record.
# The area between QNX Internal Start and QNX Internal End is controlled by
# the QNX IDE properties.

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

#===== TARGET_TYPE - the QUARC target type
endianness  = $(filter be le, $(VARIANT_LIST))
spe_variant = $(filter spe, $(VARIANT_LIST))
TARGET_TYPE = qnx_$(CPU)$(endianness:%=_%)$(spe_variant:%=_%)

#===== USEFILE - the file containing the usage message for the application. 
USEFILE=

#===== CCFLAGS - add the flags to the C compiler command line. 
CCFLAGS += -fPIC

#===== EXTRA_INCVPATH - a space-separated list of directories to search for include files.
EXTRA_INCVPATH += $(QUARC_DIR)include

#===== EXTRA_LIBVPATH - a space-separated list of directories to search for library files.
EXTRA_LIBVPATH += $(QUARC_DIR)lib/$(TARGET_TYPE)

#===== LIBS - a space-separated list of library items to be included in the link.
LIBS += quanser_communications quanser_runtime quanser_common m

include $(MKFILES_ROOT)/qmacros.mk
ifndef QNX_INTERNAL
QNX_INTERNAL=$(PROJECT_ROOT)/.qnx_internal.mk
endif
include $(QNX_INTERNAL)

include $(MKFILES_ROOT)/qtargets.mk

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))

