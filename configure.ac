#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# AC_PREREQ(2.59)
AC_INIT([elevator-control], [0.1])
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)
AC_CONFIG_HEADER(config.h)

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

ASTYLE_OPT="-A1 -T -C -S -N -L -w -Y -M -f -p --mode=c --lineend=linux --align-reference=type --align-pointer=type --suffix=none --style=ansi"
AC_SUBST(ASTYLE_OPT)

#check libsobjectizer support
#AC_MSG_CHECKING([libSobjectize support])
#AC_CHECK_HEADER([so_5/all.hpp],,exit)
SO_LIBS="-lso"
SO_CFLAGS="-Iso_5"

AC_SUBST(SO_LIBS)
AC_SUBST(SO_CFLAGS)

CXX_EXTRA_FLAGS="-Wnon-virtual-dtor -Woverloaded-virtual -Woverflow -fstack-protector"

CXXFLAGS="-I\$(top_builddir)/include $CXXFLAGS $SO_CFLAGS $CXX_EXTRA_FLAGS -funsigned-char -std=c++11 -g -D_GNU_SOURCE -pipe -O2 -pedantic -Wall"
LDFLAGS="$LDFLAGS $SO_LIBS"

AC_SUBST(LDFLAGS)
AC_SUBST(CXXFLAGS)


# check doc
DOCGEN=
DOXYGEN=
DOX=
AC_MSG_CHECKING([Generate docs])
AC_ARG_ENABLE(doc,AC_HELP_STRING([--enable-doc],[Generate documentation (with doxygen)]),[DOCGEN=yes],[DOCGEN=])

if test "x$DOCGEN" != x; then
	# check Doxygen
	AC_CHECK_PROG(DOX, doxygen, yes)
	if test "x$DOX" != x ; then
		DOXYGEN="doxygen"
	else
		AC_MSG_WARN([*** Doxygen not found. Can`t generate docs...])
		DOCGEN=
	fi
else
	AC_MSG_WARN([*** generate docs disabled... ***])
fi

AC_SUBST(DOXYGEN)
AM_CONDITIONAL( HAVE_DOXYGEN,test "x$DOX" != x )
AM_CONDITIONAL( ENABLE_DOCGEN,test "x$DOCGEN" != x )


AC_CONFIG_FILES([Makefile
				 src/Makefile
				 src/Control/Makefile
				 src/Control/tests/Makefile])
#				 docs/Makefile
#				 docs/Dox.cfg])

AC_OUTPUT
