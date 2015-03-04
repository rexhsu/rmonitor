SRCS=rmonitor.c
KMOD=rmonitor
SUBDIRS=test_tools

.include <bsd.kmod.mk>
#.include <bsd.subdir.mk>
