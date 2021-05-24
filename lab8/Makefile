
LAB_NO=08
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            	\
				\
	main.c			\
				\
	Times.txt		\
	Komentarz.txt		\
				\
	screws.ascii.pgm	\




all: executables
.PHONY: all executables

# =============================================================================

O ?= 3

CC := gcc
AR := ar
LD := $(CC)
GDB := gdb
CFLAGS := -Wall -c -g -I ./lib
OBJ := ./obj
LDFLAGS := -L $(OBJ)/  -pthread


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
	@$$(LD) -o $$@ $$(LDFLAGS) $$($(1)_obj)


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


# =============================================================================

$(eval $(call solution,zad1,  main.c))

test:
	@[ -f Times.txt ] && rm Times.txt
	
	echo >> Times.txt
	echo 1 block screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 1 block screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 2 block screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 2 block screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 4 block screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 4 block screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 8 block screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 8 block screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 16 block screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 16 block screws.ascii.pgm out.pgm >> Times.txt
	
	
	echo >> Times.txt
	echo 1 numbers screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 1 numbers screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 2 numbers screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 2 numbers screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 4 numbers screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 4 numbers screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 8 numbers screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 8 numbers screws.ascii.pgm out.pgm >> Times.txt
	
	echo >> Times.txt
	echo 16 numbers screws.ascii.pgm out.pgm >> Times.txt
	$(OBJ)/zad1 16 numbers screws.ascii.pgm out.pgm >> Times.txt



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

$(ARCHIVE): $(ARCHIVE_CONTENTS)
	@printf "%10s %s\n" TAR $(ARCHIVE)
	@tar -czf $(ARCHIVE) $(AUTHOR)

	@printf "%10s %s\n" RM $(AUTHOR)
	@rm -rf $(AUTHOR)

.PHONY: achive
archive: $(ARCHIVE)
