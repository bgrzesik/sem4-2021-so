
LAB_NO=01
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            \
	zad1/main.c         \
	\
	zad1/block_arr.h    \
	zad1/block_arr.c    \
	zad1/vp_list.h      \
	zad1/vp_list.c      \
	zad1/merge.h        \
	zad1/merge.c        \
	\
	zad2/main.c         \


all: archive

# =============================================================================


CC=gcc
LD=$(CXX)
GDB=gdb
CFLAGS=-Wall -c -g -I./zad1
LDFLAGS= 
OBJ_DIR=./obj

zad1_src=zad1/main.c zad1/block_arr.c zad1/vp_list.c zad1/merge.c
zad1_obj=$(addprefix $(OBJ_DIR)/, $(zad1_src:%.c=%.c.o))

obj/zad1.out: $(zad1_obj)
	@printf "%10s %s\n" LD $<
	@$(LD) $(LDFLAGS) $(zad1_obj) -o $@

zad2_src=zad2/main.c
zad2_obj=$(addprefix $(OBJ_DIR)/, $(zad2_src:%.c=%.c.o))

obj/zad2.out: $(zad2_obj)
	@printf "%10s %s\n" LD $<
	@$(LD) $(LDFLAGS) $(zad2_obj) -o $@


obj/%.c.o : %.c
	@mkdir -p $(dir $(OBJ_DIR)/$<)
	@printf "%10s %s\n" CC $<
	@$(CC) $(CFLAGS) $< -o $@

# =============================================================================

zad%: obj/zad%.out
	

run%: obj/zad%.out
	@printf "%10s %s\n" RUN $<
	LD_LIBRARY_PATH=$(OBJ_DIR) ./$<

gdb%: obj/zad%.out
	@printf "%10s %s\n" GDB $<
	LD_LIBRARY_PATH=$(OBJ_DIR) $(GDB) ./$<

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
