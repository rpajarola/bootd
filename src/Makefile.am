AM_CFLAGS = --pedantic -Wall -std=c99 -O2
AM_LDFLAGS =

RPC_GENERATED = generated_bootparam_prot.h
BUILT_SOURCES = $(RPC_GENERATED)

bin_PROGRAMS = bootd
bootd_SOURCES = bootd.c bootd.h bootd_const.h bootparam.c bootparam.h conf.c conf.h device.c device.h dhcp.c dhcp.h genutil.h global.h host.c host.h listener.c listener.h log.c ngenutil.c ngenutil.h parse.c rarp.c rarp.h replace.c rmp.c rmp.h service.c service.h str.c tftp.c tftp.h tokenize.c util.c util.h bootparam_prot.x $(RPC_GENERATED)
nodist_bootd_SOURCES = $(RPC_GENERATED)

generated_bootparam_prot.h: bootparam_prot.x
	@RPCGEN_PATH@ -C -N -h -o $@ $<

CLEANFILES = $(RPC_GENERATED)
MAINTAINERCLEANFILES = Makefile.in

maintainerclean-local:
	rm -rf .deps
