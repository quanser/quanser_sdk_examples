LIST=CPU
ifndef QRECURSE
QRECURSE=recurse.mk
ifdef QCONFIG
QRDIR=$(dir $(QCONFIG))
endif
endif
QUARC_DIR := $(subst \,/,$(shell $(QUARC_DIR)examples/C/communications/quarc_root.bat))
include $(QRDIR)$(QRECURSE)
