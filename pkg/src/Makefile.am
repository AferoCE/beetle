bin_PROGRAMS = beetle

# _POSIX_C_SOURCE is needed to use clock_gettime.
beetle_CFLAGS = -std=c99 -D_POSIX_SOURCE -D_POSIX_C_SOURCE=199309L -Wall -Werror

if BUILD_NUMBER_SET
    beetle_CFLAGS += -DBUILD_NUMBER=\"$(BUILD_NUMBER)\"
else
    beetle_CFLAGS += -DBUILD_NUMBER=\"local\"
endif

beetle_LDADD = -lbluetooth
beetle_SOURCES = \
    babygatt_central.c      \
    babygatt_central.h      \
    babygatt_common.c       \
    babygatt_common.h       \
    babygatt_peripheral.c   \
    babygatt_peripheral.h   \
    beetle.c                \
    beetle.h                \
    central.c               \
    central.h               \
    command.c               \
    command.h               \
    connlist.c              \
    connlist.h              \
    devicelist.c            \
    devicelist.h            \
    evloop.c                \
    evloop.h                \
    hci_beetle.c            \
    hci_beetle.h            \
    log.c                   \
    log.h                   \
    peripheral.c            \
    peripheral.h            \
    utils.c                 \
    utils.h                 \
    $(NULL)

.PHONY: build_info.h
$(beetle_SOURCES): build_info.h
build_info.h:
	$(srcdir)/make-build-info.sh $(srcdir)
