

LAB_NO=01
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile    \
	zad1/main.c \
	zad2/main.c \


all: $(ARCHIVE)

# =============================================================================

CC=gcc
LD=$(CXX)
GDB=gdb
CFLAGS=-Wall -c
LDFLAGS= 
OBJ_DIR=./obj

zad1_src=zad1/main.c zad1/tmp.c
zad1_obj=$(addprefix $(OBJ_DIR)/, $(zad1_src:%.c=%.c.o))
obj/zad1.out: $(zad1_obj)
	@printf "%10s %s\n" LD $<
	@$(LD) $(LDFLAGS) $(zad1_obj) -o $@


obj/%.c.o : %.c
	@mkdir -p $(dir $(OBJ_DIR)/$<)
	@printf "%10s %s\n" CC $<
	@$(CC) $(CFLAGS) $< -o $@

# =============================================================================

run%: obj/zad%.out
	@printf "%10s %s\n" RUN $<
	./$<

gdb%: obj/zad%.out
	@printf "%10s %s\n" GDB $<
	$(GDB) $<

# =============================================================================

.PHONY: clean
clean:
	@printf "%10s %s\n" RM $(ARCHIVE)
	@rm -f $(ARCHIVE)

	@printf "%10s %s\n" RM $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)

# =============================================================================

ARCHIVE=$(AUTHOR)-cw$(LAB_NO).tar.gz
ARCHIVE_TOP=$(AUTHOR)/cw$(LAB_NO)
ARCHIVE_CONTENTS=$(addprefix $(ARCHIVE_TOP)/, $(FILES))

$(ARCHIVE_TOP)/%:
	@printf "%10s %s\n" INSTALL $(@:$(ARCHIVE_TOP)/%=%)
	@install -C -D $(@:$(ARCHIVE_TOP)/%=%) ./$@


$(ARCHIVE): $(ARCHIVE_CONTENTS)
	@printf "%10s %s\n" TAR $(ARCHIVE)
	@tar -czf $(ARCHIVE) $(AUTHOR)

	@printf "%10s %s\n" RM $(AUTHOR)
	@rm -rf $(AUTHOR)

archive: $(ARCHIVE)
