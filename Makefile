TARGET = channel
TARGET_SANITIZE = channel_sanitize
STUDENT_OBJS += channel.o
STUDENT_OBJS += linked_list.o
OBJS += $(STUDENT_OBJS)
OBJS += buffer.o
OBJS += stress.o
OBJS += stress_send_recv.o
OBJS += test.o
LIBS += -lpthread
LIBS += -lrt

CC = /usr/bin/gcc
CFLAGS += -MMD -MP # dependency tracking flags
CFLAGS += -I./
CFLAGS += -std=gnu11 -g -Wall -Werror -Wconversion
LDFLAGS += $(LIBS)

NOT_ALLOWED += -Dsleep=sleep_not_allowed
NOT_ALLOWED += -Dusleep=usleep_not_allowed
NOT_ALLOWED += -Dnanosleep=nanosleep_not_allowed
NOT_ALLOWED += -Dclock_nanosleep=clock_nanosleep_not_allowed
NOT_ALLOWED += -Dselect=select_not_allowed
NOT_ALLOWED += -Dsem_timedwait=sem_timedwait_not_allowed
NOT_ALLOWED += -Dpthread_cond_timedwait=pthread_cond_timedwait_not_allowed
NOT_ALLOWED += -Dpthread_mutex_timedlock=pthread_mutex_timedlock_not_allowed
NOT_ALLOWED += -Dpthread_rwlock_timedrdlock=pthread_rwlock_timedrdlock_not_allowed
NOT_ALLOWED += -Dpthread_rwlock_timedwrlock=pthread_rwlock_timedwrlock_not_allowed

all: CFLAGS += -O2 # release flags
all: $(TARGET) $(TARGET_SANITIZE)

release: clean all

debug: CFLAGS += -O0 # debug flags
debug: clean $(TARGET) $(TARGET_SANITIZE)

SANITIZE_OBJS = $(OBJS:%.o=%_sanitize.o)
$(TARGET_SANITIZE): $(SANITIZE_OBJS)
	$(CC) $(CFLAGS) -fsanitize=thread -o $@ $^ $(LDFLAGS)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(STUDENT_OBJS:%.o=%_sanitize.o): CFLAGS += $(NOT_ALLOWED)
%_sanitize.o: %.c
	$(CC) $(CFLAGS) -fPIC -fsanitize=thread -c -o $@ $<

$(STUDENT_OBJS): CFLAGS += $(NOT_ALLOWED)
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

ALL_OBJS = $(OBJS) + $(SANITIZE_OBJS)
DEPS = $(ALL_OBJS:%.o=%.d)
-include $(DEPS)

clean:
	-@rm $(TARGET) $(TARGET_SANITIZE) $(ALL_OBJS) $(DEPS) 2> /dev/null || true

test:
	@chmod +x grade.py
	@sed -i -e 's/\r$$//g' *.py # dos to unix
	@sed -i -e 's/\r/\n/g' *.py # mac to unix
	@sed -i -e 's/\r$$//g' *.txt # dos to unix
	@sed -i -e 's/\r/\n/g' *.txt # mac to unix
	./grade.py
