CC = gcc
AR = ar

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Imy_libs
LDLIBS = -lm

BUILD_DIR = build
RESULTS_DIR = results
DATASET = data/wine/wine.data
TARGET = $(BUILD_DIR)/main.exe
LIBRARY = $(BUILD_DIR)/libdimred.a

LIB_SOURCES = $(wildcard my_libs/*.c)
LIB_OBJECTS = $(patsubst my_libs/%.c,$(BUILD_DIR)/%.o,$(LIB_SOURCES))

.PHONY: all run clean rebuild dirs

all: dirs $(TARGET)

run: all
	$(TARGET) $(DATASET)

rebuild: clean all

dirs:
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	@if not exist $(RESULTS_DIR) mkdir $(RESULTS_DIR)

$(TARGET): main.c $(LIBRARY)
	$(CC) $(CFLAGS) main.c -L$(BUILD_DIR) -ldimred $(LDLIBS) -o $@

$(LIBRARY): $(LIB_OBJECTS)
	$(AR) rcs $@ $^

$(BUILD_DIR)/%.o: my_libs/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
