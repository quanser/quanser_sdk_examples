LIST=CPU
ifndef QRECURSE
QRECURSE=recurse.mk
ifdef QCONFIG
QRDIR=$(dir $(QCONFIG))
endif
endif
QSDK_DIR := $(subst \,/,$(shell $(QSDK_DIR)examples/C/communications/quarc_root.bat))
include $(QRDIR)$(QRECURSE)
