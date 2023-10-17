###############################################################
# Makefile for building the msi-installation-file
#
# This Makefile works with MS NMake as shipped with MS 
#   Visual++ Studio.net Standard 2005.
#
###############################################################
# The variable BIN_DIR needs to be adjusted by the user
# before executing this makescript
# example:
# Set BIN_DIR="..\..\bin\x86\release"
###############################################################

WIX=wix build -nologo -ext WixToolset.Netfx.wixext -ext WixToolset.Util.wixext
COMMON_GUI=source

all: mysql_workbench.msi

mysql_workbench.msi: mysql_workbench.xml mysql_workbench_fragment.xml $(COMMON_GUI)\mysql_common_ui.xml
  $(WIX) -out $@ -b $(BIN_DIR) -d LICENSE_TYPE=$(LICENSE_TYPE) -d SETUP_TYPE=$(SETUP_TYPE) -d VERSION_MAIN=$(VERSION_MAIN) -d VERSION_DETAIL=$(VERSION_DETAIL) -d LICENSE_SCREEN=$(LICENSE_SCREEN) -arch $(ARCHITECTURE) mysql_workbench.xml mysql_workbench_fragment.xml

clean:
  del mysql_workbench.msi 1> nul 2> nul
  del mysql_workbench*.xml 1> nul 2> nul
