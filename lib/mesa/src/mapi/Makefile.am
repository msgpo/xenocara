# Copyright © 2013, 2014 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

SUBDIRS =
TESTS =

BUILT_SOURCES =
CLEANFILES = $(BUILT_SOURCES)

lib_LTLIBRARIES =
check_PROGRAMS =
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =

EXTRA_DIST = \
	new/genCommon.py \
	new/gen_gldispatch_mapi.py \
	es1api/gles1-symbols.txt \
	es2api/gles2-symbols.txt \
	mapi_abi.py \
	glapi/SConscript \
	glapi/registry/gl.xml \
	shared-glapi/SConscript \
	es1api/meson.build \
	es2api/meson.build \
	glapi/meson.build \
	shared-glapi/meson.build \
	meson.build

AM_CFLAGS = \
	$(PTHREAD_CFLAGS) \
	$(MSVC2013_COMPAT_CFLAGS)
AM_CPPFLAGS =							\
	$(DEFINES)						\
	$(SELINUX_CFLAGS)					\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/src					\
	-I$(top_builddir)/src/mapi				\
	-I$(top_srcdir)/src/mapi				\
	-I$(top_builddir)/src/mapi/glapi			\
	-I$(top_srcdir)/src/mapi/glapi

include Makefile.sources

MKDIR_GEN = $(AM_V_at)$(MKDIR_P) $(@D)
PYTHON_GEN = $(AM_V_GEN)$(PYTHON) $(PYTHON_FLAGS)

shared_glapi_gen_mapi_deps := \
	mapi_abi.py \
	$(wildcard $(top_srcdir)/src/mapi/glapi/gen/*.xml) \
	$(wildcard $(top_srcdir)/src/mapi/glapi/gen/*.py)

glapi_gen_gl_xml := \
	$(srcdir)/glapi/registry/gl.xml
glapi_gen_mapi_script = $(srcdir)/new/gen_gldispatch_mapi.py
glapi_gen_mapi_deps = \
	$(glapi_gen_mapi_script) \
	$(srcdir)/new/genCommon.py \
	$(glapi_gen_gl_xml)
glapi_gen_mapi = $(AM_V_GEN)$(PYTHON) $(PYTHON_FLAGS) $(glapi_gen_mapi_script)

if HAVE_SHARED_GLAPI
BUILT_SOURCES += shared-glapi/glapi_mapi_tmp.h

lib_LTLIBRARIES += shared-glapi/libglapi.la
shared_glapi_libglapi_la_SOURCES = $(MAPI_GLAPI_FILES) shared-glapi/glapi_mapi_tmp.h
shared_glapi_libglapi_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VISIBILITY_CFLAGS)
shared_glapi_libglapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_GLAPI \
	-DMAPI_ABI_HEADER=\"shared-glapi/glapi_mapi_tmp.h\"
shared_glapi_libglapi_la_LIBADD = \
	$(PTHREAD_LIBS) \
	$(SELINUX_LIBS)
shared_glapi_libglapi_la_LDFLAGS = \
	-no-undefined \
	-version-number 0:2 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

TESTS += shared-glapi-test
check_PROGRAMS += shared-glapi-test

shared_glapi_test_SOURCES = shared-glapi/tests/check_table.cpp
shared_glapi_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/gtest/include \
	-I$(top_srcdir)/src/mesa
shared_glapi_test_LDADD = \
	$(top_builddir)/src/mapi/shared-glapi/libglapi.la \
	$(top_builddir)/src/gtest/libgtest.la
endif

if REGEN_SOURCES
shared-glapi/glapi_mapi_tmp.h : glapi/gen/gl_and_es_API.xml $(shared_glapi_gen_mapi_deps)
	$(MKDIR_GEN)
	$(PYTHON_GEN) $(srcdir)/mapi_abi.py --printer shared-glapi \
		$(srcdir)/glapi/gen/gl_and_es_API.xml > $@
endif

if HAVE_OPENGL
noinst_LTLIBRARIES = glapi/libglapi.la

if HAVE_X86_ASM
if HAVE_X86_64_ASM
GLAPI_ASM_SOURCES = glapi/glapi_x86-64.S
else
GLAPI_ASM_SOURCES = glapi/glapi_x86.S
endif
endif
if HAVE_SPARC_ASM
GLAPI_ASM_SOURCES = glapi/glapi_sparc.S
endif

glapi_libglapi_la_SOURCES =
glapi_libglapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/mesa

if HAVE_APPLEDRI
glapi_libglapi_la_SOURCES += glapi/glapi_gentable.c
endif
if HAVE_WINDOWSDRI
glapi_libglapi_la_SOURCES += glapi/glapi_gentable.c
endif

if HAVE_SHARED_GLAPI
glapi_libglapi_la_SOURCES += $(MAPI_BRIDGE_FILES) glapi/glapi_mapi_tmp.h
glapi_libglapi_la_CPPFLAGS += \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"glapi/glapi_mapi_tmp.h\"
else
glapi_libglapi_la_CPPFLAGS += \
	-DMAPI_MODE_UTIL
glapi_libglapi_la_SOURCES += \
	glapi/glapi_dispatch.c \
	glapi/glapi_entrypoint.c \
	glapi/glapi_getproc.c \
	glapi/glapi_nop.c \
	glapi/glapi.c \
	glapi/glapi.h \
	glapi/glapi_priv.h \
	$(GLAPI_ASM_SOURCES) \
	$(MAPI_UTIL_FILES)

TESTS += glapi-test
check_PROGRAMS += glapi-test

glapi_test_SOURCES = glapi/tests/check_table.cpp
glapi_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/gtest/include \
	-I$(top_srcdir)/src/mesa

glapi_test_LDADD = \
	$(top_builddir)/src/mapi/glapi/libglapi.la \
	$(top_builddir)/src/gtest/libgtest.la
endif
endif

if USE_LIBGLVND
else
if HAVE_OPENGL_ES1
TESTS += es1api/ABI-check

BUILT_SOURCES += es1api/glapi_mapi_tmp.h

pkgconfig_DATA += es1api/glesv1_cm.pc

GLES_includedir = $(includedir)/GLES
GLES_include_HEADERS = \
	$(top_srcdir)/include/GLES/egl.h \
	$(top_srcdir)/include/GLES/gl.h \
	$(top_srcdir)/include/GLES/glext.h \
	$(top_srcdir)/include/GLES/glplatform.h

lib_LTLIBRARIES += es1api/libGLESv1_CM@GLES_LIB_SUFFIX@.la

es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_SOURCES = entry.c es1api/glapi_mapi_tmp.h
es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VISIBILITY_CFLAGS)
es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"es1api/glapi_mapi_tmp.h\"
es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_LIBADD = $(GLESv1_CM_LIB_DEPS)
es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_LDFLAGS = \
	-no-undefined \
	-version-number 2:0 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

es1api_libGLESv1_CM@GLES_LIB_SUFFIX@_la_LIBADD += shared-glapi/libglapi.la
endif # HAVE_OPENGL_ES1
endif # USE_LIBGLVND

if REGEN_SOURCES
es1api/glapi_mapi_tmp.h: $(glapi_gen_mapi_deps)
	$(MKDIR_GEN)
	$(glapi_gen_mapi) glesv1 $(glapi_gen_gl_xml) > $@
endif

if USE_LIBGLVND
else
if HAVE_OPENGL_ES2
TESTS += es2api/ABI-check

BUILT_SOURCES += es2api/glapi_mapi_tmp.h

pkgconfig_DATA += es2api/glesv2.pc

GLES2_includedir = $(includedir)/GLES2
GLES2_include_HEADERS = \
	$(top_srcdir)/include/GLES2/gl2.h \
	$(top_srcdir)/include/GLES2/gl2ext.h \
	$(top_srcdir)/include/GLES2/gl2platform.h
GLES3_includedir = $(includedir)/GLES3
GLES3_include_HEADERS = \
	$(top_srcdir)/include/GLES3/gl3.h \
	$(top_srcdir)/include/GLES3/gl31.h \
	$(top_srcdir)/include/GLES3/gl32.h \
	$(top_srcdir)/include/GLES3/gl3ext.h \
	$(top_srcdir)/include/GLES3/gl3platform.h

lib_LTLIBRARIES += es2api/libGLESv2@GLES_LIB_SUFFIX@.la

es2api_libGLESv2@GLES_LIB_SUFFIX@_la_SOURCES = entry.c es2api/glapi_mapi_tmp.h
es2api_libGLESv2@GLES_LIB_SUFFIX@_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VISIBILITY_CFLAGS)
es2api_libGLESv2@GLES_LIB_SUFFIX@_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"es2api/glapi_mapi_tmp.h\"
es2api_libGLESv2@GLES_LIB_SUFFIX@_la_LIBADD = $(GLESv2_LIB_DEPS)
es2api_libGLESv2@GLES_LIB_SUFFIX@_la_LDFLAGS = \
	-no-undefined \
	-version-number 2:0 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

es2api_libGLESv2@GLES_LIB_SUFFIX@_la_LIBADD += shared-glapi/libglapi.la
endif # HAVE_OPENGL_ES2
endif # USE_LIBGLVND

if REGEN_SOURCES
es2api/glapi_mapi_tmp.h: $(glapi_gen_mapi_deps)
	$(MKDIR_GEN)
	$(glapi_gen_mapi) glesv2 $(glapi_gen_gl_xml) > $@
endif

include $(top_srcdir)/install-lib-links.mk

if NEED_KHRPLATFORM
khrdir = $(includedir)/KHR
khr_HEADERS = $(top_srcdir)/include/KHR/khrplatform.h
endif
