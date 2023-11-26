###############################################################################
#                                                                             #
# MAKEFILE for chudnovsky                                                     #
#                                                                             #
# (c) Guy Wilson 2023                                                         #
#                                                                             #
###############################################################################

# Directories
SOURCE = src
BUILD = build
DEP = dep

# What is our target
TARGET = chudnovsky

# Tools
C = gcc
LINKER = gcc

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CFLAGS = -c -O2 -Wall -pedantic -I /opt/homebrew/include
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
STDLIBS = -pthread
EXTLIBS = -lgmp
COMPILE.c = $(C) $(CFLAGS) $(DEPFLAGS) -o $@
LINK.o = $(LINKER) -L /opt/homebrew/lib $(STDLIBS) -o $@

CSRCFILES = $(wildcard $(SOURCE)/*.c)
OBJFILES = $(patsubst $(SOURCE)/%.c, $(BUILD)/%.o, $(CSRCFILES))
DEPFILES = $(patsubst $(SOURCE)/%.c, $(DEP)/%.d, $(CSRCFILES))

all: $(TARGET)

# Compile C/C++ source files
#
$(TARGET): $(OBJFILES)
	$(LINK.o) $^ $(EXTLIBS)

$(BUILD)/%.o: $(SOURCE)/%.c
$(BUILD)/%.o: $(SOURCE)/%.c $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

.PRECIOUS = $(DEP)/%.d
$(DEP)/%.d: ;

-include $(DEPFILES)

clean:
	rm -r $(BUILD)
	rm -r $(DEP)
	rm $(TARGET)
