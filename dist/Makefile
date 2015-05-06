.PHONY: pkg all clean dist

include ../version.mk

DIST_DIR = connection-pool

PKG_NAME = connection-pool_$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)_$(OS_VER).tar.bz2


dist: $(DIST_DIR)
	# prep header files
	cp -r ../*.h $(DIST_DIR)/include/

$(DIST_DIR):
	mkdir -p $(DIST_DIR)/include

pkg: dist
	$(info -------------------------)
	$(info Package: $(PKG_NAME))
	$(info -------------------------)
	tar cjvf $(PKG_NAME) $(DIST_DIR)

clean:
	$(RM) -r $(DIST_DIR) *.tar.bz2

all: dist pkg

