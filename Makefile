# specify build tools
CELL_BUILD_TOOLS	=	SNC
#explicitly set some cell sdk defaults
CELL_SDK		?=	/usr/local/cell
# CELL_GPU_TYPE (currently RSX is only one option)  
CELL_GPU_TYPE		=	RSX    
#CELL_PSGL_VERSION is debug, dpm or opt  
CELL_PSGL_VERSION	=	opt  

#Python binary - only useful for PSL1ght scripts
PYTHONBIN		= python2.7

CELL_MK_DIR		?= $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk
#CELL_HOST_PATH		?= $(CELL_SDK)/host-win32
MKFSELF			= $(CELL_HOST_PATH)/bin/make_fself
MKFSELF_NPDRM		= $(CELL_HOST_PATH)/bin/make_fself_npdrm
MKPKG_NPDRM		= $(CELL_HOST_PATH)/bin/make_package_npdrm

# Geohot CFW defines
MKSELF_GEOHOT		= make_self_npdrm
MKPKG_PSLIGHT		= old-buildtools/PS3Py/pkg.py
PKG_FINALIZE		= package_finalize

STRIP			= $(CELL_HOST_PATH)/ppu/bin/ppu-lv2-strip
COPY			= cp
MOVE			= mv

PPU_OPTIMIZE_LV		= -O2

UTILS_DIR		= ./utils
SRC_DIR			= ./src
GENESIS_PLUS_API_DIR	= ./src/genplusgx
CELL_FRAMEWORK_DIR	= ./src/cellframework

PPU_SRCS 		:= $(SRC_DIR)/GenesisPlus.cpp \
			$(SRC_DIR)/ps3video.cpp \
			$(SRC_DIR)/ps3input.cpp \
			$(SRC_DIR)/menu.cpp \
			$(CELL_FRAMEWORK_DIR)/input/cellInput.cpp \
			$(CELL_FRAMEWORK_DIR)/graphics/PSGLGraphics.cpp \
			$(CELL_FRAMEWORK_DIR)/fileio/FileBrowser.cpp\
			$(CELL_FRAMEWORK_DIR)/threads/thread.cpp\
			$(CELL_FRAMEWORK_DIR)/threads/cond.cpp\
			$(CELL_FRAMEWORK_DIR)/threads/mutex.cpp\
			$(CELL_FRAMEWORK_DIR)/threads/scoped_lock.cpp\
			$(CELL_FRAMEWORK_DIR)/audio/resampler.cpp\
			$(CELL_FRAMEWORK_DIR)/audio/quadratic_resampler.cpp\
			$(CELL_FRAMEWORK_DIR)/audio/buffer.c \
			$(CELL_FRAMEWORK_DIR)/audio/librsound.c \
			$(CELL_FRAMEWORK_DIR)/utility/OSKUtil.cpp \
			$(SRC_DIR)/conf/conffile.cpp \
			$(SRC_DIR)/conf/reader.cpp \
			$(GENESIS_PLUS_API_DIR)/config.c \
			$(GENESIS_PLUS_API_DIR)/genesis.c \
			$(GENESIS_PLUS_API_DIR)/vdp.c \
			$(GENESIS_PLUS_API_DIR)/state.c \
			$(GENESIS_PLUS_API_DIR)/render.c \
			$(GENESIS_PLUS_API_DIR)/system.c \
			$(GENESIS_PLUS_API_DIR)/unzip.c \
			$(GENESIS_PLUS_API_DIR)/fileio.c \
			$(GENESIS_PLUS_API_DIR)/gen_io.c \
			$(GENESIS_PLUS_API_DIR)/gen_input.c \
			$(GENESIS_PLUS_API_DIR)/loadrom.c \
			$(GENESIS_PLUS_API_DIR)/mem68k.c \
			$(GENESIS_PLUS_API_DIR)/memz80.c \
			$(GENESIS_PLUS_API_DIR)/membnk.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/areplay.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/cart_hw.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/eeprom.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/ggenie.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/sram.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/svp/ssp16.c \
			$(GENESIS_PLUS_API_DIR)/cart_hw/svp/svp.c \
			$(GENESIS_PLUS_API_DIR)/ntsc/md_ntsc.c \
			$(GENESIS_PLUS_API_DIR)/ntsc/sms_ntsc.c \
			$(GENESIS_PLUS_API_DIR)/sound/Fir_Resampler.c \
			$(GENESIS_PLUS_API_DIR)/sound/eq.c \
			$(GENESIS_PLUS_API_DIR)/sound/sound.c \
			$(GENESIS_PLUS_API_DIR)/sound/ym2612.c \
			$(GENESIS_PLUS_API_DIR)/sound/sn76489.c \
			$(GENESIS_PLUS_API_DIR)/sound/blip.c \
			$(GENESIS_PLUS_API_DIR)/z80/z80.c \
			$(GENESIS_PLUS_API_DIR)/m68k/m68kcpu.c \
			$(GENESIS_PLUS_API_DIR)/m68k/m68kops.c \
			$(UTILS_DIR)/zlib/adler32.c \
			$(UTILS_DIR)/zlib/compress.c \
			$(UTILS_DIR)/zlib/crc32.c \
			$(UTILS_DIR)/zlib/deflate.c \
			$(UTILS_DIR)/zlib/gzio.c \
			$(UTILS_DIR)/zlib/infblock.c \
			$(UTILS_DIR)/zlib/infcodes.c \
			$(UTILS_DIR)/zlib/inffast.c \
			$(UTILS_DIR)/zlib/inflate.c \
			$(UTILS_DIR)/zlib/inftrees.c \
			$(UTILS_DIR)/zlib/infutil.c \
			$(UTILS_DIR)/zlib/trees.c \
			$(UTILS_DIR)/zlib/uncompr.c \
			$(UTILS_DIR)/zlib/zutil.c 


# define for PS3 SDK 3.41, comment this for 1.92
PPU_CFLAGS		+= -DPS3_SDK_3_41
PPU_CXXFLAGS		+= -DPS3_SDK_3_41

PPU_TARGET		= genesisplus.elf

# PPU_CPPFLAGS - is this used anywhere?
PPU_CPPFLAGS		+= -DWORDS_BIGENDIAN  -D'VERSION="Genesis Plus PS3"' -DPSGL

#PPU_ASFLAGS		+=

# debugging
#PPU_CFLAGS		+= -DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002
#PPU_CXXFLAGS		+= -DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002
PPU_SRCS		+= $(CELL_FRAMEWORK_DIR)/network/network.cpp \
			$(CELL_FRAMEWORK_DIR)/logger/NetLogger.cpp \
			$(CELL_FRAMEWORK_DIR)/logger/Logger.cpp \
			$(CELL_FRAMEWORK_DIR)/network/Socket.cpp \
			$(CELL_FRAMEWORK_DIR)/network/TCPSocket.cpp \
			$(CELL_FRAMEWORK_DIR)/network/Poller.cpp

PPU_CXXFLAGS		+= -I. -Isrc/genplusgx/sound -Isrc/genplusgx/cart_hw -Isrc/genplusgx/cart_hw/svp -Isrc/genplusgx/ntsc -Isrc/genplusgx/z80 -Isrc/genplusgx/m68k -Isrc -Iutils/zlib -Isrc/cellframework/threads -Isrc/cellframework/input -Isrc/conf

ifeq ($(CELL_BUILD_TOOLS),SNC)
PPU_CFLAGS		+= -Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
			-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER
PPU_CXXFLAGS		+= -Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
			-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER
else
PPU_CFLAGS		+= -funroll-loops -DGCC_COMPILER
PPU_CXXFLAGS		+= -funroll-loops -DGCC_COMPILER
PPU_LDFLAGS		+= -finline-limit=5000
PPU_LDFLAGS		+= -Wl
endif

PPU_LDLIBS		+= -L. -L$(CELL_SDK)/target/ppu/lib/PSGL/RSX/opt -ldbgfont -lPSGL -lPSGLcgc -lcgc \
			-lgcm_cmd -lgcm_sys_stub -lresc_stub -lm -lio_stub -lfs_stub -lsysutil_stub \
			-lsysmodule_stub -laudio_stub -lpthread -lnet_stub

include $(CELL_MK_DIR)/sdk.target.mk

.PHONY: pkg
#standard pkg packaging
pkg: $(PPU_TARGET)
	$(STRIP) $(PPU_TARGET)
	$(MKFSELF_NPDRM) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN
	$(COPY) -r src/cellframework/extra/shaders pkg/USRDIR/
	$(MKPKG_NPDRM) pkg/package.conf pkg

#massively reduced filesize using MKSELF_GEOHOT - use this for normal jailbreak builds
.PHONY: pkg-signed
pkg-signed: $(PPU_TARGET)
	$(MKSELF_GEOHOT) $(PPU_TARGET) EBOOT.BIN GENP00001
	$(MOVE) -f EBOOT.BIN pkg/USRDIR/EBOOT.BIN
	$(COPY) -r src/cellframework/extra/shaders pkg/USRDIR/
	$(PYTHONBIN) $(MKPKG_PSLIGHT) --contentid IV0002-GENP00001_00-SAMPLE0000000001 pkg/ genplusgx-ps3.pkg

#use this to create a PKG for use with Geohot CFW 3.55
.PHONY: pkg-signed-cfw
pkg-signed-cfw:
	$(MKSELF_GEOHOT) $(PPU_TARGET) EBOOT.BIN GENP00001
	$(MOVE) -f EBOOT.BIN pkg/USRDIR/EBOOT.BIN
	$(COPY) -r src/cellframework/extra/shaders pkg/USRDIR/
	$(PYTHONBIN) $(MKPKG_PSLIGHT) --contentid IV0002-GENP00001_00-SAMPLE0000000001 pkg/ genplusgx-ps3-geohot.pkg
	$(PKG_FINALIZE) genplusgx-ps3-geohot.pkg
