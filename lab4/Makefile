
LAB_NO=04
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            	\





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

$(1)_obj := $(addprefix $(OBJ)/$(1).out/, $(2:%.c=%.o))
$(foreach src, $(2), 
$(addprefix $(OBJ)/$(1).out/, $(src:%.c=%.o)): $(src)
	    @mkdir -p $$(dir $$@)
	    @printf "%10s %s\n" CC $$<
	    @$(CC) $(CFLAGS) $$< -o $$@ $3
)

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


#$(eval $(call solution,z1, zad1/main.c))

$(eval $(call solution,zad2_onstack, zad2/onstack.c))
$(eval $(call solution,zad2_no_onstack, zad2/onstack.c, -DNO_ONSTACK))
$(eval $(call solution,zad2_nodefer, zad2/nodefer.c))
$(eval $(call solution,zad2_no_nodefer, zad2/nodefer.c, -DNO_NODEFER))
$(eval $(call solution,zad2_restart, zad2/restart.c))
$(eval $(call solution,zad2_no_restart, zad2/restart.c, -DNO_RESTART))

$(eval $(call solution,zad3a_sender, zad3a/sender.c))
$(eval $(call solution,zad3a_catcher, zad3a/catcher.c))

run_zad3a: zad3a_sender zad3a_catcher
	@killall -SIGKILL zad3a_catcher 2>&1 1>/dev/null || true
	$(OBJ)/zad3a_catcher &
	$(OBJ)/zad3a_sender `pidof zad3a_catcher` $(ARGS)


#obj/%.o : %.c
#	@mkdir -p $(dir $@)
#	@printf "%10s %s\n" CC $<
#	@$(CC) $(CFLAGS) $< -o $@

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
