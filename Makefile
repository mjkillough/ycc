CFLAGS = -std=c11 -W -Wall -Wextra -pedantic -g

SOURCE = src/
BUILD = build/
TARGET = build/ycc

OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

run: $(TARGET)
	$(TARGET)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS)

$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir $@

clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
