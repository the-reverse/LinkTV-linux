#EXTRA_CFLAGS := -Werror

obj-$(CONFIG_OPROFILE) += oprofile.o

DRIVER_OBJS = $(addprefix ../../../drivers/oprofile/, \
		oprof.o cpu_buffer.o buffer_sync.o \
		event_buffer.o oprofile_files.o \
		oprofilefs.o oprofile_stats.o \
		timer_int.o )

oprofile-y				:= $(DRIVER_OBJS) common.o

oprofile-$(CONFIG_VENUS_OPROFILE_PERFORMANCE_COUNTER)		+= op_model_mipsxx.o
oprofile-$(CONFIG_CPU_MIPS64)		+= op_model_mipsxx.o
oprofile-$(CONFIG_CPU_RM9000)		+= op_model_rm9000.o
oprofile-$(CONFIG_VENUS_OPROFILE_CLOCK)	+= op_model_venus.o
