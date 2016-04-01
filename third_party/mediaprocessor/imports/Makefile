# <COPYRIGHT_TAG>
EXCLUDE_DIRS=gpac
DIRS:=$(shell find . -maxdepth 1 -type d)
DIRS:=$(basename $(patsubst ./%,%,$(DIRS)))
DIRS:=$(filter-out $(EXCLUDE_DIRS),$(DIRS))

ASTA_DEP=ffmpeg #gpac
VCSA_WAV_DEP=libvpx speex freetype2
VCSA_AUD_DEP=speex
VCSA_VID_DEP=libvpx freetype2
ADEC_DEP=ipp speex
XCODER_DEP=live555 freetype2
XCODER_SW_DEP=ffmpeg
SNVA_DEP=ffmpeg opencv
VIOA_DEP=libvpx freetype2
ON_DEMAND_SERVER_DEP=live555

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

asta:$(ASTA_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir asta; \
	done

fei:
	@echo "fei feature"

vcsa:$(VCSA_WAV_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

vcsa_audio:$(VCSA_AUD_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

vcsa_video:$(VCSA_VID_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

adec:$(ADEC_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder:$(XCODER_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

xcoder_sw:$(XCODER_SW_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir xcoder_sw; \
	done

snva:$(SNVA_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir snva; \
	done

vioa:$(VIOA_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done

on_demand_server:$(ON_DEMAND_SERVER_DEP)
	@for dir in $^; \
		do $(MAKE) -C $$dir; \
	done
	@echo "Building On Demand Server Done"

clean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done
