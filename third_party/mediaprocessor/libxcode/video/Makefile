# <COPYRIGHT_TAG>

LSPCI=lspci
ifeq ("/usr/bin/lspci", "$(wildcard /usr/bin/lspci)")
	LSPCI=/usr/bin/lspci
else ifeq ("/sbin/lspci", "$(wildcard /sbin/lspci)")
	LSPCI=/sbin/lspci
endif


#DEVICEID=$(shell $(LSPCI) -nn | grep VGA | grep Intel | cut -d: -f 4 | cut -d']' -f 1)
MEDIASDK_OUTPUT=/opt/intel
#MEDIASDK_INSTALL_DIR=$(MEDIASDK_OUTPUT)/mediasdk/lib64/8086/$(DEVICEID)/
#since mss v16.4.2.1 PV, libdispatch_share.a should be used
ifeq ($(LIBDISPATCH_ENABLE),1)
MEDIASDK_INSTALL_DIR=/opt/intel/mediasdk/lib/lin_x64/
MFX_LIBS=-L$(MEDIASDK_INSTALL_DIR) -ldispatch_shared -Wl,-rpath=$(MEDIASDK_INSTALL_DIR) -lva -lva-drm -lva-x11
EXTRA_FLAGS+=-DMFX_DISPATCHER_EXPOSED_PREFIX
else
MEDIASDK_INSTALL_DIR=$(shell find $(MEDIASDK_OUTPUT)/mediasdk/lib64/ -name libmfxhw64.so -exec dirname {} \; | sed -n '1p')
MFX_LIBS=-L$(MEDIASDK_INSTALL_DIR) -lmfxhw64 -Wl,-rpath=$(MEDIASDK_INSTALL_DIR) -lva -lva-drm -lva-x11
endif
MFX_INCLUDE_PATH=$(MEDIASDK_OUTPUT)/mediasdk/include/
LIBS+=$(MFX_LIBS)
INCLUDES_PATH+=-I$(MFX_INCLUDE_PATH)

ifeq ($(VPX_ENABLE),1)
VPX_INSTALL_DIR=../../imports/libvpx
VPX_INCLUDE_PATH=$(VPX_INSTALL_DIR)/libvpx
VPX_LIBS=-L$(VPX_INSTALL_DIR)/build -lvpx -Wl,-rpath=$(VPX_INSTALL_DIR)/build
EXTRA_FLAGS+=-DENABLE_VPX_CODEC
LIBS+=$(VPX_LIBS)
INCLUDES_PATH+=-I$(VPX_INCLUDE_PATH)
endif

ifeq ($(RENDER_ENABLE),1)
EXTRA_FLAGS+=-DLIBVA_SUPPORT -DLIBVA_X11_SUPPORT
endif

ifeq ($(STR_ENABLE),1)
FREETYPE_INSTALL_DIR=../../imports/freetype2
FREETYPE_INCLUDE_PATH=$(FREETYPE_INSTALL_DIR)/freetype2/include/freetype2
#FREETYPE_INCLUDE_PATH=../../../freetype2/include
FREETYPE_LIBS=-L$(FREETYPE_INSTALL_DIR)/freetype2/lib -lfreetype -Wl,-rpath=$(FREETYPE_INSTALL_DIR)/freetype2/lib
EXTRA_FLAGS+=-DENABLE_STRING_CODEC
LIBS+=$(FREETYPE_LIBS)
INCLUDES_PATH+=-I$(FREETYPE_INCLUDE_PATH)
endif

ifeq ($(PIC_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_PICTURE_CODEC
endif

ifeq ($(FEI_ENABLE),1)
EXTRA_FLAGS+= -DMSDK_FEI
endif

ifeq ($(VA_ENABLE),1)
VA_INSTALL_DIR=../../imports/opencv
VA_INCLUDE_PATH=$(VA_INSTALL_DIR)/include
VA_LIBS=-L$(VA_INSTALL_DIR)/lib -lopencv_core -lopencv_highgui -lopencv_objdetect -lopencv_flann \
				-lopencv_calib3d -lopencv_features2d -lopencv_ml -lopencv_video \
				-lopencv_imgcodecs -lopencv_videoio -Wl,-rpath=$(VA_INSTALL_DIR)/lib
EXTRA_FLAGS+= -DENABLE_VA -DLIBVA_SUPPORT -DLIBVA_X11_SUPPORT
LIBS+=$(VA_LIBS)
OPENCL_INSTALL_DIR=/usr/lib64
OPENCL_LIBS=-L$(OPENCL_INSTALL_DIR) -lOpenCL -Wl,-rpath=$(OPENCL_INSTALL_DIR) -L/opt/intel/opencl -Wl,-rpath=/opt/intel/opencl
LIBS+=$(OPENCL_LIBS)
INCLUDES_PATH+=-I$(VA_INCLUDE_PATH) -I/opt/intel/opencl/include
endif

ifeq ($(SW_ENABLE),1)
FFMPEG_INSTALL_DIR=../../imports/ffmpeg
FFMPEG_INCLUDE_PATH=$(FFMPEG_INSTALL_DIR)/include
#FFMPEG_LIBS=-L$(FFMPEG_INSTALL_DIR)/lib -lavcodec -lavformat -lavutil -Wl,-rpath=$(FFMPEG_INSTALL_DIR)/lib
EXTRA_FLAGS+=-DENABLE_SW_CODEC -D__STDC_FORMAT_MACROS -D__STDC_CONSTANT_MACROS
#LIBS+=$(FFMPEG_LIBS)
INCLUDES_PATH+=-I$(FFMPEG_INCLUDE_PATH)
endif

ifeq ($(RAW_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_RAW_CODEC
endif

ifeq ($(SMTA_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_RAW_DECODE -DSUPPORT_INTERLACE -DSUPPORT_SMTA
endif

ifeq ($(ASTA_CONFIG_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_ASTA_CONFIG
endif

DIST_DIR=../../dist
BASE_LIBS=-L $(DIST_DIR) -lxcodebase -Wl,-rpath='$$ORIGIN'

LIBS+=$(BASE_LIBS)

include ../common.mk
