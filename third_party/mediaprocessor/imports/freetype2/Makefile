# <COPYRIGHT_TAG>

DIR = freetype2
DST = ../../dist

all:
ifeq ($(DST)/libfreetype.so, $(wildcard $(DST)/libfreetype.so))
	@echo "libfreetype.so already built"
else
ifeq ($(DIR)/include/freetype2/freetype.h, $(wildcard $(DIR)/include/freetype2/freetype.h))
	@echo "folder freetype2 already exist"
else
	mkdir $(DIR)/include/freetype2
endif
	mkdir $(DIR)/include/freetype2/freetype
	mkdir $(DIR)/include/freetype2/freetype/config
	cd $(DIR) && ./configure --prefix=`pwd`
	$(MAKE) -C $(DIR)
	$(MAKE) -C $(DIR) install
	cp -d $(DIR)/lib/libfreetype.so* $(DST)
endif

clean:
	$(MAKE) -C $(DIR) distclean
	rm -f $(DST)/libfreetype.so*
