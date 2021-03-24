
LAB_NO=02
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            	\
	long.txt            	\
	dane.txt            	\
				\
	zad1/lib_main.c       	\
	zad1/sys_main.c       	\
	pomiar_zad_1.txt       	\
				\
	zad2/lib_main.c       	\
	zad2/sys_main.c       	\
	pomiar_zad_2.txt       	\
				\
	zad3/lib_main.c       	\
	zad3/sys_main.c       	\
	pomiar_zad_3.txt       	\
				\
	zad4/lib_main.c       	\
	zad4/sys_main.c       	\
	pomiar_zad_4.txt       	\
				\
	zad5/lib_main.c       	\
	zad5/sys_main.c       	\
	pomiar_zad_5.txt       	\



all: executables
.PHONY: all executables

# =============================================================================

O ?= 3

CC := gcc
AR := ar
LD := $(CC)
GDB := gdb
CFLAGS := -Wall -c -g
OBJ := ./obj
LDFLAGS := -L $(OBJ)/ -lm


define solution

$(1)_obj := $(addprefix $(OBJ)/, $(2:%.c=%.o))

$(1): $(OBJ)/$(1)
$(OBJ)/$(1): $$($(1)_obj)
	@printf "%10s %s\n" LD $(1)
	@$$(LD) -o $$@ $$(LDFLAGS) $$<


run_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" RUN $(1)
	@LD_LIBRARY_PATH=$(OBJ) $$< $$(ARGS)

gdb_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" GDB $(1)
	@LD_LIBRARY_PATH=./obj/ $(GDB) $$< $(ARGS)

valgrind_$(1): $(OBJ)/$(1)
	@printf "%10s %s\n" VALGRIND $(1)
	@LD_LIBRARY_PATH=./obj/ valgrind                                 \
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


$(eval $(call solution,zad1_lib, zad1/lib_main.c))
$(eval $(call solution,zad1_sys, zad1/sys_main.c))

$(eval $(call solution,zad2_lib, zad2/lib_main.c))
$(eval $(call solution,zad2_sys, zad2/sys_main.c))

$(eval $(call solution,zad3_lib, zad3/lib_main.c))
$(eval $(call solution,zad3_sys, zad3/sys_main.c))

$(eval $(call solution,zad4_sys, zad4/sys_main.c))
$(eval $(call solution,zad4_lib, zad4/lib_main.c))

$(eval $(call solution,zad5_sys, zad5/sys_main.c))
$(eval $(call solution,zad5_lib, zad5/lib_main.c))


measure: executables
	# ====================================================================

	sync
	sleep 2
	echo "Lib" > pomiar_zad_1.txt
	$(OBJ)/zad1_lib long.txt dane.txt 1>/dev/null 2>> pomiar_zad_1.txt

	sync
	sleep 2
	echo "Sys" >> pomiar_zad_1.txt
	$(OBJ)/zad1_sys long.txt dane.txt 1>/dev/null 2>> pomiar_zad_1.txt

	# ====================================================================

	sync
	sleep 2
	echo "Lib" > pomiar_zad_2.txt
	$(OBJ)/zad2_lib 0 dane.txt 1>/dev/null 2>> pomiar_zad_2.txt

	sync
	sleep 2
	echo "Sys" >> pomiar_zad_2.txt
	$(OBJ)/zad2_sys 0 dane.txt 1>/dev/null 2>> pomiar_zad_2.txt

	# ====================================================================

	sync
	sleep 2
	echo "Lib" > pomiar_zad_3.txt
	$(OBJ)/zad3_lib 1>/dev/null 2>> pomiar_zad_3.txt


	sync
	sleep 2
	echo "Sys" >> pomiar_zad_3.txt
	$(OBJ)/zad3_sys 1>/dev/null 2>> pomiar_zad_3.txt
	rm a.txt b.txt c.txt

	# ====================================================================

	sync
	sleep 2
	echo "Lib" > pomiar_zad_4.txt
	$(OBJ)/zad4_sys long.txt tmp.txt C XXX 1>/dev/null 2>> pomiar_zad_4.txt
	
	sync
	sleep 2
	echo "Sys" >> pomiar_zad_4.txt
	$(OBJ)/zad4_lib long.txt tmp.txt C XXX 1>/dev/null 2>> pomiar_zad_4.txt
	rm tmp.txt

	# ====================================================================

	sync
	sleep 2
	echo "Lib" > pomiar_zad_5.txt
	$(OBJ)/zad5_sys long.txt tmp.txt 1>/dev/null 2>> pomiar_zad_5.txt

	sync
	sleep 2
	echo "Sys" >> pomiar_zad_5.txt
	$(OBJ)/zad5_lib long.txt tmp.txt 1>/dev/null 2>> pomiar_zad_5.txt
	rm tmp.txt

	# ====================================================================


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
