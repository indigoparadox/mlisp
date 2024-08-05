
# vim: ft=make noexpandtab

C_FILES := src/main.c
RETROFLAT_DOS_MEM_LARGE := 1
#GLOBAL_DEFINES += -DMPARSER_TRACE_LVL=1
GLOBAL_DEFINES += -DMPARSER_TRACE_NAMES
GLOBAL_DEFINES += -DMLISP_TRACE_LVL=1
GLOBAL_DEFINES += -DMLISP_EXEC_TRACE_LVL=1
#GLOBAL_DEFINES += -DMLISP_PARSE_TRACE_LVL=1
#GLOBAL_DEFINES += -DMDATA_TRACE_LVL=1
GLOBAL_DEFINES += -DMLISP_DEBUG_TRACE=20

include maug/Makefile.inc

# Target-specific options.
.PHONY: clean

all: mlisp.ale mlisp.sdl mlispd.exe mlispw.exe mlispnt.exe mlisp.html

# Unix (Allegro)

$(eval $(call TGTUNIXALE,mlisp))

# Unix (SDL)

$(eval $(call TGTUNIXSDL,mlisp))

# WASM

$(eval $(call TGTWASMSDL,mlisp))

# DOS

$(eval $(call TGTDOSBIOS,mlisp))

$(eval $(call TGTDOSALE,mlisp))

# WinNT

$(eval $(call TGTWINNT,mlisp))

# Win386

$(eval $(call TGTWIN386,mlisp))

# Clean

clean:
	rm -rf $(CLEAN_TARGETS)

