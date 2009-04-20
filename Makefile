
PROJECT = s-os

SRCS = $(wildcard src/*.c) $(wildcard fMSX/Z80/*.c)
OBJDIR = obj
OBJS = $(addprefix $(OBJDIR)/,$(subst .c,.o,$(notdir $(SRCS))))

INCLUDES = -IfMSX/Z80
CFLAGS = -O2 -DLSB_FIRST
CC = gcc
LINK = gcc

all:	$(PROJECT).exe

run:
	./$(PROJECT).exe

$(PROJECT).exe:	objs
	$(LINK) -o $@ $(OBJS)

objs:	$(OBJDIR) $(OBJS)

$(OBJDIR):
	mkdir $(OBJDIR)

obj/%.o:	src/%.c
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<

obj/%.o:	fMSX/Z80/%.c
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $<


clean:
	rm -rf obj
	rm $(PROJECT).exe
