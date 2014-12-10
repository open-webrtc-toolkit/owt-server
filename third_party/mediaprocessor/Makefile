# <COPYRIGHT_TAG>

DIRS = imports libxcode
SRCS = libxcode

ifeq (README, $(wildcard README))
	TARGET=help
else
ifeq (README.ADEC, $(wildcard README.ADEC))
	TARGET=adec
endif

ifeq (README.VCSA, $(wildcard README.VCSA))
	TARGET=vcsa
endif

ifeq (README.VCSA_AUDIO, $(wildcard README.VCSA_AUDIO))
	TARGET=vcsa_audio
endif

ifeq (README.VCSA_VIDEO, $(wildcard README.VCSA_VIDEO))
	TARGET=vcsa_video
endif

ifeq (README.ASTA, $(wildcard README.ASTA))
	TARGET=asta
endif

ifeq (README.XCODER, $(wildcard README.XCODER))
	TARGET=xcoder
endif

ifeq (README.XCODER_SW, $(wildcard README.XCODER_SW))
	TARGET=xcoder_sw
endif

ifeq (README.SNVA, $(wildcard README.SNVA))
	TARGET=snva
endif

ifeq (README.VIOA, $(wildcard README.VIOA))
	TARGET=vioa
endif
endif

all:$(TARGET)

help:
	@echo "Usage: make <TARGET> [ver=release/debug]"
	@echo "Description:"
	@echo "       TARGET : asta, to build ASTA codes"
	@echo "              : vcsa, to build VCSA without IPP"
	@echo "              : vcsa_audio, to build VCSA without IPP and video module"
	@echo "              : vcsa_video, to build VCSA without IPP and audio module"
	@echo "              : adec, to build VCSA with IPP"
	@echo "              : xcoder, to build file and stream codes"
	@echo "              : xcoder_sw, to build software file codes"
	@echo "              : fei, to build FEI samples"
	@echo "              : snva, to build SNVA codes."
	@echo "              : vioa, to build VIOA codes."
	@echo "              : on_demand_server, to build On Demand Server."
	@echo "              : clean, to clean the builds except imports directory"
	@echo "              : distclean, to clean all the builds"
	@echo "       ver=release/debug, release or debug version to build"
	@echo "Example:"
	@echo "       \"make vcsa ver=debug\", to build VCSA debug version"

asta:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir asta ASTA_CONFIG_ENABLE=1; \
	done

vcsa:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa; \
	done

adec:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir adec; \
	done

vcsa_audio:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa_audio; \
	done

vcsa_video:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vcsa_video; \
	done

xcoder:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir xcoder RAW_ENABLE=1 STR_ENABLE=1; \
	done

xcoder_sw:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir xcoder_sw RAW_ENABLE=1 SW_ENABLE=1; \
	done

fei:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir fei; \
	done

snva:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir snva VA_ENABLE=1; \
	done

vioa:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir vioa; \
	done

on_demand_server:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir on_demand_server; \
	done

distclean:
	@for dir in $(DIRS); \
		do $(MAKE) -C $$dir clean; \
	done

clean:
	@for dir in $(SRCS); \
		do $(MAKE) -C $$dir clean; \
	done
