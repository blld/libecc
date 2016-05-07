
machine ?= $(shell $(CC) -dumpmachine)

root := .$(machine:%=/%)
sources := $(wildcard ../src/*.c) $(wildcard ../src/builtin/*.c)
objects := $(sources:../src/%.c=$(root)/object/%.o)
library := $(root)/lib/libecc.a
binary := $(root)/bin/ecc

#

.PHONY: debug paths
debug : debug := -DDEBUG=1
debug : optim := -O0 -g
debug : lto   := 

warn  ?= -Wall
optim ?= -Os -fstrict-aliasing
debug ?= -DNDEBUG=1
lto   ?= $(shell echo "main(){}" | $(CC) -flto -o/dev/null -xc - >/dev/null 2>&1 && echo "-flto")
libs  ?= $(shell echo "main(){}" | $(CC) -lm -o/dev/null -xc - >/dev/null 2>&1 && echo "-lm")

CFLAGS += $(warn) $(optim) $(debug) $(lto)

##

all: $(root)/bin $(root)/lib $(root)/object/builtin $(library) $(binary)
debug: all
clean:
	@rm -rfv $(root)/bin $(root)/lib $(root)/object
	@$(machine:%=rm -rfv ./%)

$(root)/bin $(root)/lib $(root)/object/builtin:
	@mkdir -p $@

test: all
	$(binary) --test

$(library): $(objects)
	@echo "   [AR] $@"
	@$(AR) rcs $@ $^
	@$(AR) d $@ main.o

$(binary): $(objects)
	@echo "   [LD] $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(libs) $^ -o $@

$(root)/object/%.o: ../src/%.c
	@echo "   [CC] $@"
	@$(CC) $(CFLAGS) -I../src/ -c $< -o $@
