
LAB_NO=03
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            	\
				\
	zad1/main.c		\
				\
	zad2/main.c		\
				\
	zad3/main.c		\
				\
	lib/lib.h		\
				\
	lib/block_arr.c		\
	lib/block_arr.h		\
				\
	lib/merge.c		\
	lib/merge.h		\
				\
	lib/vp_list.c		\
	lib/vp_list.h		\





all: executables
.PHONY: all executables

# =============================================================================

O ?= 3

CC := gcc
AR := ar
LD := $(CC)
GDB := gdb
CFLAGS := -Wall -c -g -I./lib -DLIB_STATIC
OBJ := ./obj
LDFLAGS := -L $(OBJ)/ -lm


define solution

$(1)_obj := $(addprefix $(OBJ)/, $(2:%.c=%.o))

$(1): $(OBJ)/$(1)
$(OBJ)/$(1): $$($(1)_obj)
	@printf "%10s %s\n" LD $(1)
	@$$(LD) -o $$@ $$(LDFLAGS) $$^


run_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" RUN $(1)
	@LD_LIBRARY_PATH=$(OBJ) $$< $$(ARGS)

gdb_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" GDB $(1)
	@LD_LIBRARY_PATH=./obj/ $(GDB) $$< $(ARGS)

valgrind_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" VALGRIND $(1)
	@LD_LIBRARY_PATH=./obj/ valgrind                                \
			--leak-check=full				\
			-q						\
			--trace-children=yes			        \
			$(OBJ)/$(1) $(ARGS)

callgrind_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" CALLGRIND $(1)
	@LD_LIBRARY_PATH=./obj/ valgrind 			        \
			--tool=callgrind                                \
			--callgrind-out-file=$(OBJ)/$(1).callgrind      \
			$$< $(ARGS)

	callgrind_annotate $(OBJ)/$(1).callgrind

executables: $(1)

.PHONY: $(1) run_$(1) gdb_$(1) valgrind_$(1) callgrind_$(1)

endef


$(eval $(call solution,z1, zad1/main.c))
$(eval $(call solution,z2, zad2/main.c lib/block_arr.c lib/merge.c lib/vp_list.c))
$(eval $(call solution,z3, zad3/main.c))


measure: executables
	echo TODO

obj/%.o : %.c
	@mkdir -p $(dir $@)
	@printf "%10s %s\n" CC $<
	@$(CC) $(CFLAGS) $< -o $@

# =============================================================================

.PHONY: clean
clean:
	@printf "%10s %s\n" RM $(ARCHIVE)
	@rm -f $(ARCHIVE)

	@printf "%10s %s\n" RM ./obj/
	@rm -rf ./obj/

# =============================================================================

ARCHIVE=$(AUTHOR)-cw$(LAB_NO).tar.gz
ARCHIVE_TOP=$(AUTHOR)/cw$(LAB_NO)
ARCHIVE_CONTENTS=$(addprefix $(ARCHIVE_TOP)/, $(FILES))

$(ARCHIVE_TOP)/%: %
	@printf "%10s %s\n" INSTALL $(@:$(ARCHIVE_TOP)/%=%)
	@install -C -D $(@:$(ARCHIVE_TOP)/%=%) ./$@

$(ARCHIVE): measure $(ARCHIVE_CONTENTS)
	@printf "%10s %s\n" TAR $(ARCHIVE)
	@tar -czf $(ARCHIVE) $(AUTHOR)

	@printf "%10s %s\n" RM $(AUTHOR)
	@rm -rf $(AUTHOR)

.PHONY: achive
archive: $(ARCHIVE)
