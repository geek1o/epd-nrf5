CC = gcc
CFLAGS = -Wall -IGUI -D__HEAP_SIZE=32768
LDFLAGS = -lgdi32 -mwindows

LOCALE ?= ru
ifeq ($(LOCALE),zh)
GUI_SRCS = GUI/fonts.c GUI/GUI.c GUI/Lunar.c
else
GUI_SRCS = GUI/fonts_ru.c GUI/GUI_RU.c GUI/calendar.c
endif
SRCS = GUI/Adafruit_GFX.c GUI/u8g2_font.c $(GUI_SRCS) emulator.c
OBJS = $(SRCS:.c=.o)
TARGET = emulator.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)