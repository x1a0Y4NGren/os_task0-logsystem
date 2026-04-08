CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -pthread
TARGET = logsystem
SRCS = src/main.c src/buffer.c src/log_ai.c src/output.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) analysis.csv alerts.log

.PHONY: all clean
