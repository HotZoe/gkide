AUTOENV_DIR=${PWD}
GKIDE_SRC_DIR=../..

all:
	@echo "make ubuntu"
	@echo "make debian"
	@echo "make macosx"
	@echo "make windows(TODO)"

ubuntu: debian

debian:
	@cd ${GKIDE_SRC_DIR} && ${AUTOENV_DIR}/debian.sh

macosx:
	@cd ${GKIDE_SRC_DIR} && ${AUTOENV_DIR}/macosx.sh

windows:
	@cd ${GKIDE_SRC_DIR} && ${AUTOENV_DIR}/windows.sh

