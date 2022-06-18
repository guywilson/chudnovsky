###############################################################################
#                                                                             #
# MAKEFILE for xword                                                          #
#                                                                             #
# (c) Guy Wilson 2021                                                         #
#                                                                             #
###############################################################################

# Directories
SOURCE = src
BUILD = build
DEP = dep

# What is our target
TARGET = chudnovsky

# Tools
C = gcc-11
LINKER = gcc-11

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CFLAGS = -c -O2 -Wall -pedantic
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
STDLIBS = -lgmp
EXTLIBS = -lgmp

COMPILE.c = $(C) $(CFLAGS) $(DEPFLAGS) -o $@
LINK.o = $(LINKER) -o $@

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
