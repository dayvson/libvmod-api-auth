INCLUDES = -I$(VARNISHSRC)/include -I$(VARNISHSRC) 

vmoddir = $(VMODDIR)
vmod_LTLIBRARIES = libvmod_authorization.la

libvmod_authorization_la_LDFLAGS = -module -export-dynamic -avoid-version -shared -lmongoc -lmhash -lhiredis

libvmod_authorization_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
    database.h \
    database.c \
    database_mongo.h \
    database_mongo.c \
    database_redis.h \
    database_redis.c \
	vmod_authorization.c

vcc_if.c vcc_if.h: $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_authorization.vcc
	@PYTHON@ $(VARNISHSRC)/lib/libvmod_std/vmod.py $(top_srcdir)/src/vmod_authorization.vcc

VMOD_TESTS = tests/*.vtc
.PHONY: $(VMOD_TESTS)

tests/*.vtc:
	$(VARNISHSRC)/bin/varnishtest/varnishtest -Dvarnishd=$(VARNISHSRC)/bin/varnishd/varnishd -Dvmod_topbuild=$(abs_top_builddir) $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_authorization.vcc \
	$(VMOD_TESTS)

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h
