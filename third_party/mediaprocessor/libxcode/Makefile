# <COPYRIGHT_TAG>

EXCLUDE_DIRS=include
DIRS:=$(shell find . -maxdepth 1 -type d)
DIRS:=$(basename $(patsubst ./%,%,$(DIRS)))
DIRS:=$(filter-out $(EXCLUDE_DIRS),$(DIRS))


DIR_BASE=base
DIR_AUDIO=audio
DIR_VIDEO=video
DIR_FORMATS=formats
DIR_OCL=ocl

help:
	@echo "make asta"
	@echo "make vcsa"
	@echo "make vcsa_audio"
	@echo "make vcsa_video"
	@echo "make adec"
	@echo "make xcoder"
	@echo "make xcoder_sw"
	@echo "make fei"
	@echo "make snva"
	@echo "make vioa"
	@echo "make on_demand_server"

asta:libxcodebase libxcodevideo
	@echo "Building ASTA Done"

vcsa:libxcodebase libxcodevideovpx libxcodewav
	@echo "Building VCSA Done"

adec:libxcodebase libxcodeaudio
	@echo "Building VCSA with ADec Done"

vcsa_audio:libxcodebase libxcodewav
	@echo "Building VCSA without Video Done"

vcsa_video:libxcodebase libxcodevideovpx
	@echo "Building VCSA without Audio Done"

snva:libxcodebase libxcodevideo libocl
	@echo "Building SNVA Done"

xcoder:libxcodebase libxcodevideo libxcodeformats
	@echo "Building XCODER Done"

xcoder_sw:libxcodebase libxcodevideo
	@echo "Building XCODER_SW Done"

vioa:libxcodebase libxcodevideoio
	@echo "Building VIOA Done"

fei:libxcodebase libxcodevideofei
	@echo "Building XCODER Done"

on_demand_server:
	@echo "Building On Demand Server Done"

libxcodebase:$(DIR_BASE)
	$(MAKE) -C $(DIR_BASE)

libxcodevideo:$(DIR_VIDEO)
	$(MAKE) -C $(DIR_VIDEO)

libxcodevideovpx:$(DIR_VIDEO)
	$(MAKE) -C $(DIR_VIDEO) VPX_ENABLE=1 STR_ENABLE=1 PIC_ENABLE=1

libxcodevideoio:$(DIR_VIDEO)
	$(MAKE) -C $(DIR_VIDEO) VPX_ENABLE=1 STR_ENABLE=1 PIC_ENABLE=1

libxcodevideofei:$(DIR_VIDEO)
	$(MAKE) -C $(DIR_VIDEO) FEI_ENABLE=1

libxcodeaudio:$(DIR_AUDIO)
	$(MAKE) -C $(DIR_AUDIO) ADEC_ENABLE=1

libxcodewav:$(DIR_AUDIO)
	$(MAKE) -C $(DIR_AUDIO) ADEC_ENABLE=0

libxcodeformats:$(DIR_FORMATS)
	$(MAKE) -C $(DIR_FORMATS)

libocl:$(DIR_OCL)
	$(MAKE) -C $(DIR_OCL)

clean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done

