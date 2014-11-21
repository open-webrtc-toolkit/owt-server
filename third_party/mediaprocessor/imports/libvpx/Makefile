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
	$(MAKE) -C $(DIR) clean
	rm $(DST)/libvpx.so*
