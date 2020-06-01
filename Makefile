# Makefile for Fusion power ports

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2
CPPFLAGS = -fPIC -Wall -Wextra -O2
LDFLAGS = -lstdc++  -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt 
RM = rm -f
STRIP = strip
TARGET = mcp3421

SRCS = mcp3421.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: ${TARGET}

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ ${LDFLAGS}
	$(STRIP) $@ >/dev/null 2>&1  || true

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET} ${OBJS} *~
