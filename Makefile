# specify build tools
CELL_BUILD_TOOLS	=	gcc
#explicitly set some cell sdk defaults
CELL_SDK		?=	/usr/local/cell
# CELL_GPU_TYPE (currently RSX is only one option)  
CELL_GPU_TYPE		=	RSX    
#CELL_PSGL_VERSION is debug, dpm or opt  
CELL_PSGL_VERSION	=	opt  

CELL_MK_DIR		?=	$(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk
CELL_HOST_PATH		?=	$(CELL_SDK)/host-win32
MKFSELF			=	$(CELL_HOST_PATH)/bin/make_fself
MKFSELF_NPDRM		=	$(CELL_HOST_PATH)/bin/make_fself_npdrm
MKPKG_NPDRM		=	$(CELL_HOST_PATH)/bin/make_package_npdrm
STRIP			=	$(CELL_HOST_PATH)/ppu/bin/ppu-lv2-strip
COPY			=	cp
PPU_OPTIMIZE_LV		=	-O2

PPU_SRCS := ./src/GenesisPlus.cpp \
	./src/ps3video.cpp \
	./src/menu.cpp \
	./src/cellframework/input/cellInput.cpp \
	./src/cellframework/graphics/PSGLGraphics.cpp \
	./src/cellframework/fileio/FileBrowser.cpp\
	./src/cellframework/threads/thread.cpp\
	./src/cellframework/threads/cond.cpp\
	./src/cellframework/threads/mutex.cpp\
	./src/cellframework/threads/scoped_lock.cpp\
	./src/cellframework/audio/resampler.cpp\
	./src/cellframework/audio/quadratic_resampler.cpp\
	./src/cellframework/audio/buffer.c \
	./src/cellframework/audio/librsound.c \
	./src/cellframework/utility/OSKUtil.cpp \
	./src/conf/conffile.cpp \
	./src/conf/reader.cpp \
	./src/genplusgx/config.c \
	./src/genplusgx/genesis.c \
	./src/genplusgx/vdp.c \
	./src/genplusgx/state.c \
	./src/genplusgx/render.c \
	./src/genplusgx/system.c \
	./src/genplusgx/unzip.c \
	./src/genplusgx/fileio.c \
	./src/genplusgx/gen_io.c \
	./src/genplusgx/gen_input.c \
	./src/genplusgx/loadrom.c \
	./src/genplusgx/mem68k.c \
	./src/genplusgx/memz80.c \
	./src/genplusgx/membnk.c \
	./src/genplusgx/cart_hw/datel.c \
	./src/genplusgx/cart_hw/cart_hw.c \
	./src/genplusgx/cart_hw/eeprom.c \
	./src/genplusgx/cart_hw/ggenie.c \
	./src/genplusgx/cart_hw/sram.c \
	./src/genplusgx/cart_hw/svp/ssp16.c \
	./src/genplusgx/cart_hw/svp/svp.c \
	./src/genplusgx/ntsc/md_ntsc.c \
	./src/genplusgx/ntsc/sms_ntsc.c \
	./src/genplusgx/sound/Fir_Resampler.c \
	./src/genplusgx/sound/eq.c \
	./src/genplusgx/sound/sound.c \
	./src/genplusgx/sound/ym2612.c \
	./src/genplusgx/sound/sn76489.c \
	./src/genplusgx/z80/z80.c \
	./src/genplusgx/m68k/m68kcpu.c \
	./src/genplusgx/m68k/m68kops.c \
	./utils/zlib/adler32.c \
	./utils/zlib/compress.c \
	./utils/zlib/crc32.c \
	./utils/zlib/deflate.c \
	./utils/zlib/gzio.c \
	./utils/zlib/infblock.c \
	./utils/zlib/infcodes.c \
	./utils/zlib/inffast.c \
	./utils/zlib/inflate.c \
	./utils/zlib/inftrees.c \
	./utils/zlib/infutil.c \
	./utils/zlib/trees.c \
	./utils/zlib/uncompr.c \
	./utils/zlib/zutil.c 

PPU_TARGET		=	genesisplus.elf

PPU_CPPFLAGS		+=	-DWORDS_BIGENDIAN  -D'VERSION="Genesis Plus PS3"' -DPS3_SDK_3_41 -DPSGL

PPU_ASFLAGS		+=

PPU_LDFLAGS		=	-Wl -finline-limit=5000

# debugging
PPU_CFLAGS		+=	-DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002
PPU_CXXFLAGS		+=	-DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002
PPU_SRCS		+=	src/cellframework/network/network.cpp src/cellframework/logger/NetLogger.cpp src/cellframework/logger/Logger.cpp src/cellframework/network/Socket.cpp src/cellframework/network/TCPSocket.cpp src/cellframework/network/Poller.cpp

PPU_CXXFLAGS		+=	-I. -Isrc/genplusgx/sound -Isrc/genplusgx/cart_hw -Isrc/genplusgx/cart_hw/svp -Isrc/genplusgx/ntsc -Isrc/genplusgx/z80 -Isrc/genplusgx/m68k -Isrc -Iutils/zlib -Isrc/cellframework/threads -Isrc/cellframework/input -Isrc/conf
PPU_LIBS		    += $(CELL_TARGET_PATH)/ppu/lib/libgcm_cmd.a \
					$(CELL_TARGET_PATH)/ppu/lib/libgcm_sys_stub.a $(CELL_TARGET_PATH)/ppu/lib/libfs_stub.a
PPU_LDLIBS		+= 	-L. -L$(CELL_SDK)/target/ppu/lib/PSGL/RSX/opt -ldbgfont -lPSGL -lPSGLcgc -lcgc -lgcm_cmd -lgcm_sys_stub -lresc_stub -lm -lio_stub -lfs_stub -lsysutil_stub -lsysmodule_stub -laudio_stub -lpthread -lnet_stub

include $(CELL_MK_DIR)/sdk.target.mk

.PHONY: pkg
pkg: $(PPU_TARGET)
	$(STRIP) $(PPU_TARGET)
	$(MKFSELF_NPDRM) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN
	$(COPY) -r src/cellframework/extra/shaders pkg/USRDIR/
	$(MKPKG_NPDRM) pkg/package.conf pkg
