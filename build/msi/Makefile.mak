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
# Set BIN_DIR="..\..\bin\windows"
###############################################################
CANDLE= ..\..\..\mysql-gui-win-res\bin\wix\candle -nologo -ext WiXNetFxExtension -ext WixUtilExtension
LIGHT= ..\..\..\mysql-gui-win-res\bin\wix\light -nologo -ext WiXNetFxExtension -ext WixUtilExtension
COMMON_GUI=source

all: mysql_workbench.msi

mysql_workbench.msi: mysql_workbench.wixobj mysql_workbench_fragment.wixobj
  $(LIGHT) -b $(BIN_DIR)  $** -out $@

mysql_workbench.wixobj: mysql_workbench.xml $(COMMON_GUI)\mysql_common_ui.xml
  $(CANDLE) mysql_workbench.xml -out $@ -dLICENSE_TYPE=$(LICENSE_TYPE) -dSETUP_TYPE=$(SETUP_TYPE) -dVERSION_MAIN=$(VERSION_MAIN) -dVERSION_DETAIL=$(VERSION_DETAIL) -dLICENSE_SCREEN=$(LICENSE_SCREEN)

mysql_workbench_fragment.wixobj: mysql_workbench_fragment.xml
  $(CANDLE) $** -out $@ -dLICENSE_TYPE=$(LICENSE_TYPE) -dSETUP_TYPE=$(SETUP_TYPE) -dVERSION_MAIN=$(VERSION_MAIN) -dVERSION_DETAIL=$(VERSION_DETAIL) -dLICENSE_SCREEN=$(LICENSE_SCREEN)

clean:
  del *.wixobj 1> nul 2> nul
  del mysql_workbench.msi 1> nul 2> nul
  del mysql_workbench*.xml 1> nul 2> nul



