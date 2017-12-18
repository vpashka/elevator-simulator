# чтобы потом задействовать и в тестах, вынесем в отдльную noinst либу
noinst_LTLIBRARIES = libControl.la
libControl_la_SOURCES = Config.cc Elevator.cc CPanel.cc Control.cc Display.cc Doors.cc

bin_PROGRAMS = elevator-control

elevator_control_LDADD   = libControl.la
elevator_control_SOURCES =  main.cc

style:
	astyle $(ASTYLE_OPT) *.cc *.h
