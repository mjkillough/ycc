CFLAGS = -std=c11 -W -Wall -Wextra -pedantic -g

BUILD = build/

SOURCE = src/
TARGET = build/ycc
OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))

TEST_SOURCE = tests/
TEST_TARGET = build/test
TEST_OBJECTS := $(patsubst $(TEST_SOURCE)%.c,$(BUILD)tests/%.o,$(wildcard $(TEST_SOURCE)*.c))

run: $(TARGET)
	$(TARGET)

all: $(TARGET)

test: $(TEST_TARGET)
	-$(TEST_TARGET)

$(TARGET): $(OBJECTS)
	@$(CC) -o $(TARGET) $(OBJECTS)

$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS)
	@$(CC) -o $(TEST_TARGET) $(TEST_OBJECTS)

$(BUILD)tests/%.o: $(TEST_SOURCE)%.c $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $@/tests

clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
