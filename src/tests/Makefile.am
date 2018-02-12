TESTS = zerotokei

check_PROGRAMS = zerotokei

AM_CXXFLAGS = \
	-iquote .. -DPKGDATADIR='"$(pkgdatadir)"' \
	$(SDL2_CFLAGS) $(SDL2_image_CFLAGS) \
	$(LAVF_CFLAGS) $(LAVC_CFLAGS) $(LSWR_CFLAGS) $(LAVU_CFLAGS) \
	$(PANGOCAIRO_CFLAGS) $(CAIRO_CFLAGS)

zerotokei_SOURCES = zerotokei.cc
zerotokei_LDADD = ../liboshu.a $(SDL2_LIBS)
