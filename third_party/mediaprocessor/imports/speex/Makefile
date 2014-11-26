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
	$(MAKE) -C $(DIR) clean
	rm $(DST)/libspeexdsp.so*
	rm $(DST)/libspeex.so*
