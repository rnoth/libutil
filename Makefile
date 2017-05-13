CC	?= cc
CFLAGS	+= -pipe -D_POSIX_C_SOURCE -std=c99 -pedantic -Wall -Wextra \
	   -Wno-missing-field-initializers -Wno-unused-parameter \
	   -Warray-bounds -Wno-switch -Wmissing-prototypes \
	   -fstrict-aliasing -fstrict-overflow \
	   -fdata-sections -ffunction-sections -fno-exceptions \
	   -fno-unwind-tables -fno-asynchronous-unwind-tables \
	   -fno-stack-protector
LDFLAGS += -lc -Wl,--sort-section=alignment -Wl,--sort-common

SRC	!= find . -maxdepth 1 -name "*.c"
OBJ	:= $(SRC:.c=.c.o)
TESTS	!= find tests -name "*-test.c"

all:: deps.mk $(NAME) $(TESTS:.c=)

ifndef NDEBUG
CFLAGS += -O0 -g -Werror
else
LDFLAGS += -Wl,--gc-section
CFLAGS += -O3 -flto
endif

-include deps.mk

deps.mk: $(SRC) $(TESTS)
	@echo making deps.mk
	@$(CC) -M $+ | sed -e 's/\.o/.c.o/' -e 's/\(.*\)-test.c.o/\1-test/' > deps.mk

%.c.o: %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

tests/%-test: tests/%-test.c %.c $(OBJ) unit.h
	@echo CCLD -o $@
	@$(CC) $(CFLAGS) -Wno-missing-prototypes -Wno-unused-variable -Wno-unused-function $(LDFLAGS) -o $@ $< $(OBJ)
	@$@ || true
	@echo

test:
	@for test in tests/*-test; do "$$test"; echo; done;

clean:
	@rm -f *.o $(NAME) tests/*-test core vgcore.*
	@echo cleaning

.PHONY: clean test
.SECONDARY: $(OBJ) $(TESTS)
