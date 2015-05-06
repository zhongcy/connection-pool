.PHONY: pkg all clean dist

OS_VER = $(shell (ver=$$(awk '{print $$3}' /etc/redhat-release); echo "CentOS$${ver}"; ))

DIST_DIR = connection-pool

PKG_NAME = connection-pool-$(OS_VER).tar.bz2


dist: $(DIST_DIR)
	# prep header files
	cp -r ../*.h $(DIST_DIR)/include/active911/

$(DIST_DIR):
	mkdir -p $(DIST_DIR)/include/active911/

pkg: dist
	$(info -------------------------)
	$(info Package: $(PKG_NAME))
	$(info -------------------------)
	tar cjvf $(PKG_NAME) $(DIST_DIR)

clean:
	$(RM) -r $(DIST_DIR) *.tar.bz2

all: dist pkg

