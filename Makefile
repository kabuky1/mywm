CC = gcc
CFLAGS = -Wall -Wextra -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -pie \
         -Wformat -Wformat-security -Werror=format-security \
         -fno-strict-aliasing -fno-common \
         -Wcast-align -Wunused-parameter -Wpointer-arith \
         -Wnested-externs -Winline -Wwrite-strings
LDFLAGS = -Wl,-z,relro,-z,now -Wl,-z,noexecstack
LIBS = -lX11

SRC = wm.c
OBJ = $(SRC:.c=.o)
TARGET = wm

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /home/kabuky/.local/bin

.PHONY: clean install
