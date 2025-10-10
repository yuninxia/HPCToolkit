# SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
#
# SPDX-License-Identifier: Apache-2.0

DEP_NUMS = 01 02 03 04 05 06 07 08 09 10 11 12 13
DLOPEN_NUMS = 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15

DEP_LIBS = $(DEP_NUMS:%=libdeptls%.so)
DEP_LINKS = $(DEP_NUMS:%=-ldeptls%)

DLOPEN_LIBS = $(DLOPEN_NUMS:%=libdlopentls%.so)

.PHONY: default $(DEP_NUMS) $(DLOPEN_NUMS) clean run

default: run

clean:
	rm -f $(DEP_LIBS) $(DLOPEN_LIBS) libaudit.so dtvstress

libdeptls%.so: tlslib.c
	gcc -shared -fPIC -o $@ -DINSTANCE=$(shell echo $@ | cut -c 10-11) -DLIBTYPE=dep tlslib.c

libdlopentls%.so: tlslib.c
	gcc -shared -fPIC -o $@ -DINSTANCE=$(shell echo $@ | cut -c 13-14) -DLIBTYPE=dlopen tlslib.c

libaudit.so: trivial-auditor.c
	gcc -shared -fPIC -o $@ $<

dtvstress: dtvstress.c $(DEP_LIBS) Makefile
	gcc -o $@ dtvstress.c -ldl -lpthread -g -Wl,-rpath,`pwd` -L`pwd` $(DEP_LINKS)

run: dtvstress libaudit.so $(DLOPEN_LIBS)
	@echo
	@echo Running without LD_AUDIT
	./dtvstress
	@echo
	@echo Running With LD_AUDIT
	LD_AUDIT=`pwd`/libaudit.so ./dtvstress
