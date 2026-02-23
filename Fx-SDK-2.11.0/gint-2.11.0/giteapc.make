# giteapc: version=1 depends=Lephenixnoir/sh-elf-gcc,Lephenixnoir/fxsdk,Lephenixnoir/OpenLibm,Vhex-Kernel-Core/fxlibc

-include giteapc-config.make

configure:
	@ fxsdk build-fx -c $(GINT_CMAKE_OPTIONS)
	@ fxsdk build-cg -c $(GINT_CMAKE_OPTIONS)
	@ fxsdk build-fxg3a -c $(GINT_CMAKE_OPTIONS)

build:
	@ fxsdk build-fx
	@ fxsdk build-cg
	@ fxsdk build-fxg3a

install:
	@ fxsdk build-fx install
	@ fxsdk build-cg install
	@ fxsdk build-fxg3a install

uninstall:
	@ if [ -e build-fx/install_manifest.txt ]; then \
	     xargs rm -f < build-fx/install_manifest.txt; \
          fi
	@ if [ -e build-cg/install_manifest.txt ]; then \
	     xargs rm -f < build-cg/install_manifest.txt; \
          fi
	@ if [ -e build-fxg3a/install_manifest.txt ]; then \
	     xargs rm -f < build-fxg3a/install_manifest.txt; \
          fi

.PHONY: configure build install uninstall
