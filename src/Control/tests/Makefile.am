noinst_PROGRAMS = tests

tests_LDADD   = $(top_builddir)/src/Control/libControl.la
tests_CPPFLAGS = -I$(top_builddir)/src/Control
tests_SOURCES = TestControlAgent.cc \
tests.cc \
test_elevator.cc \
test_doors.cc \
test_display.cc \
test_control.cc

check:
	$(CURDIR)/tests
