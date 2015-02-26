.include <bsd.kmod.mk>

SRCS=rmonitor.c
KMOD=rmonitor
SUBDIRS=test_tools

all: subdir
subdir:
	@for dir in ${SUBDIRS}; do \
		cd $$dir; \
		make; \
	done

clean: subdir_clean
subdir_clean:
	@for dir in ${SUBDIRS}; do \
		cd $$dir; \
		make clean; \
	done

