SRCS=rmonitor.c
KMOD=rmonitor
SUBDIRS=test_tools

all:
	@for dir in ${SUBDIRS}; do \
		cd $$dir; \
		make; \
	done

clean:
	@for dir in ${SUBDIRS}; do \
		cd $$dir; \
		make clean; \
	done

.include <bsd.kmod.mk>
