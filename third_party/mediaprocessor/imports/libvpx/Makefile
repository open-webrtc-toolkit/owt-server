# <COPYRIGHT_TAG>

DIR = build
DST = ../../dist

all:
ifeq ($(DST)/libvpx.so, $(wildcard $(DST)/libvpx.so))
	@echo "libvpx already built"
else
	cd $(DIR) && ../libvpx/configure --enable-shared --enable-vp8 --disable-vp9
	$(MAKE) -C $(DIR)
	cp -d $(DIR)/libvpx.so* $(DST)
endif

clean:
ifeq ($(DIR)/Makefile, $(wildcard $(DIR)/Makefile))
	$(MAKE) -C $(DIR) clean
else
endif
	rm -f $(DST)/libvpx.so*
