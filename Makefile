# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall --std=c99 -pthread

# Executable name
EXEC := proj4

# Source and binary directories
SRC := src
BIN := bin

# Source and object files
SOURCE_EXT := c
SOURCES := $(wildcard $(SRC)/*.$(SOURCE_EXT))
OBJECTS := $(SOURCES:$(SRC)/%.$(SOURCE_EXT)=$(BIN)/%.o)

# Generate necessary directories
dirs:
	@mkdir -p $(SRC)
	@mkdir -p $(BIN)

# Generate object files
$(BIN)/%.o: $(SRC)/%.$(SOURCE_EXT) dirs
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo File $@ built

$(EXEC): $(OBJECTS)
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@
	@echo Project $@ built

# Remove generated files
clean:
	@rm -f $(EXEC)
	@rm -rf $(BIN)
	@echo Project files cleaned