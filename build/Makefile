
binary := ./bin/libecc
sources := $(wildcard ../src/*.c) $(wildcard ../src/object/*.c)
objects := $(sources:../src/%.c=./object/%.o)

#

ifeq ($(std),)
	std := -std=c99
	warn ?= -Wall -pedantic
else
	warn ?= -Wall
endif

opts ?= -Os -fomit-frame-pointer -fstrict-aliasing
flto ?= -flto
libs ?= -lm

ifeq ($(debug),)
debug += -DNDEBUG=1
endif

CFLAGS += $(std) $(warn) $(opts) $(flto) $(debug)

##

all: ./object/object ./bin $(binary)

clean:
	rm -rf ./bin ./object

test: $(binary)
	$(binary) --test

./object/object ./bin:
	@mkdir -p $@

$(binary): $(objects)
	@echo "   [LD] $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(libs) $^ -o $@

./object/%.o: ../src/%.c
	@echo "   [CC] $@"
	@$(CC) $(CFLAGS) -I../src/ -c $< -o $@
