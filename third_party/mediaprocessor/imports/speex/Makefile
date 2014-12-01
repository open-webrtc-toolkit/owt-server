# <COPYRIGHT_TAG>

DIR = speex
DST = ../../dist

all:
ifeq ($(DST)/libspeex.so, $(wildcard $(DST)/libspeex.so))
	@echo "libspeex.so already built"
else
	cd $(DIR) && ./configure -q
	$(MAKE) -C $(DIR)
	cp -d $(DIR)/libspeex/.libs/libspeexdsp.so* $(DST)
	cp -d $(DIR)/libspeex/.libs/libspeex.so* $(DST)
endif

clean:
ifeq ($(DIR)/Makefile, $(wildcard $(DIR)/Makefile))
	$(MAKE) -C $(DIR) clean
else
endif
	rm -f $(DST)/libspeexdsp.so*
	rm -f $(DST)/libspeex.so*
