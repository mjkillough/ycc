CFLAGS = -std=c11 -W -Wall -Wextra -pedantic -g

BUILD = build/
BIN = bin/

SOURCE = src/
TARGET = $(BIN)ycc
OBJECTS := $(patsubst $(SOURCE)%.c,$(BUILD)%.o,$(wildcard $(SOURCE)*.c))
OBJECTS := $(filter-out $(BUILD)main.o, $(OBJECTS))

TEST_SOURCE = tests/
TEST_TARGET = $(BIN)test
TEST_OBJECTS := $(patsubst $(TEST_SOURCE)%.c,$(BUILD)tests/%.o,$(wildcard $(TEST_SOURCE)*.c))

run: $(TARGET)
	$(TARGET)

all: $(TARGET) $(TEST_TARGET)

test: $(TEST_TARGET)
	-$(TEST_TARGET)

review: $(TEST_TARGET)
	-$(TEST_TARGET) review

$(TARGET): $(OBJECTS) $(BUILD)main.o $(BIN)
	@$(CC) -o $(TARGET) $(OBJECTS) $(BUILD)main.o

$(BUILD)%.o: $(SOURCE)%.c $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS) $(OBJECTS) $(BIN)
	@$(CC) -o $(TEST_TARGET) $(TEST_OBJECTS) $(OBJECTS)

$(BUILD)tests/%.o: $(TEST_SOURCE)%.c $(BUILD)
	@$(CC) $(CFLAGS) -I$(SOURCE) -c $< -o $@

$(BUILD):
	@mkdir -p $@/tests

$(BIN):
	@mkdir -p $@/tests

clean :
	-rm -rf $(BUILD)
	-rm -f $(TARGET)
