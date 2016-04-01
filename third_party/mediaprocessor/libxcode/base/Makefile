# <COPYRIGHT_TAG>

ifeq ($(SW_ENABLE),1)
EXTRA_FLAGS+=-DENABLE_SW_CODEC
endif

ifeq ($(VA_ENABLE),1)
EXTRA_FLAGS+= -DENABLE_VA
endif
LIBS+=-lpthread -lrt
include ../common.mk

