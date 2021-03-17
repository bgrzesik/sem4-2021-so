
LAB_NO=01
AUTHOR=GrzesikBartłomiej

FILES=\
	Makefile            \
	zad1/main.c         \
	                    \
	zad1/lib.h          \
	zad1/block_arr.h    \
	zad1/block_arr.c    \
	zad1/vp_list.h      \
	zad1/vp_list.c      \
	zad1/merge.h        \
	zad1/merge.c        \
	                    \
	zad2/main.c         \
			    \
	results3a.txt       \
	results3b.txt       \


all: executables

# =============================================================================

O ?= 3
VER ?= static

CC := gcc
AR := ar
LD := $(CC)
GDB := gdb
CFLAGS := -Wall -c -g -I./zad1
LDFLAGS := -L ./obj/

opts := 0 1 2 3 s

define zad_targets

$(foreach o, $(opts),
    zad$(1)_static_o$(o)  := $$(addprefix ./obj/, $$(zad$(1)_src:%.c=%.static.o$(o)))
    zad$(1)_shared_o$(o)  := $$(addprefix ./obj/, $$(zad$(1)_src:%.c=%.static.o$(o)))
    zad$(1)_dynamic_o$(o) := $$(addprefix ./obj/, $$(zad$(1)_src:%.c=%.dynamic.o$(o)))

    obj/zad$(1).static.o$(o): $$(zad$(1)_static_o$(o)) obj/liblib_static$(o).a
	    @printf "%10s %s\n" LD $$@
	    @$$(LD) -o $$@ $$(zad$(1)_static_o$(o)) $$(LDFLAGS) -llib_static$(o)

    obj/zad$(1).shared.o$(o): $$(zad$(1)_shared_o$(o)) obj/liblib_shared$(o).so
	    @printf "%10s %s\n" LD $$@
	    @$$(LD) -o $$@ $$(zad$(1)_shared_o$(o)) $$(LDFLAGS) -llib_shared$(o)

    obj/zad$(1).dynamic.o$(o): $$(zad$(1)_dynamic_o$(o)) obj/liblib_shared$(o).so
	    @printf "%10s %s\n" LD $$@
	    @$$(LD) -o $$@ $$(zad$(1)_dynamic_o$(o)) $$(LDFLAGS) -ldl


    execs += obj/zad$(1).static.o$(o) \
	     obj/zad$(1).shared.o$(o) \
	     obj/zad$(1).dynamic.o$(o) 

    zad$(1).static: obj/zad$(1).static.o$(o)
    zad$(1).shared: obj/zad$(1).shared.o$(o) 
    zad$(1).dynamic: obj/zad$(1).dynamic.o$(o)

)

zad$(1): obj/zad$(1).$(VER).o$(O)

endef


zad1_src=zad1/main.c 
$(eval $(call zad_targets,1))

zad2_src=zad2/main.c
$(eval $(call zad_targets,2))


lib_src=zad1/merge.c zad1/block_arr.c zad1/vp_list.c

define objs
$(foreach o, $(opts),
    lib_static_$(o) := $$(addprefix ./obj/, $$(lib_src:%.c=%.static.o$(o)))
    lib_shared_$(o) := $$(addprefix ./obj/, $$(lib_src:%.c=%.shared.o$(o)))


    obj/liblib_static$(o).a: $$(lib_static_$(o))
	    @printf "%10s %s\n" AR $$@
	    @$$(AR) rcs $$@ $$(lib_static_$(o))

    obj/liblib_shared$(o).so: $$(lib_shared_$(o))
	    @printf "%10s %s\n" LD $$@
	    @$$(LD) -o $$@ $$(lib_shared_$(o)) $(LDFLAGS) -shared

    obj/%.static.o$(o) : %.c
	    @mkdir -p $$(dir ./obj/$$<)
	    @printf "%10s %s\n" CC $$<
	    @$$(CC) $$(CFLAGS) $$< -o $$@ -D LIB_STATIC -O$(o)

    obj/%.shared.o$(o) : %.c
	    @mkdir -p $$(dir ./obj/$$<)
	    @printf "%10s %s\n" CC $$<
	    @$$(CC) -fPIC $$(CFLAGS) $$< -o $$@ -D LIB_SHARED -O$(o)

    obj/%.dynamic.o$(o) : %.c
	    @mkdir -p $$(dir ./obj/$$<)
	    @printf "%10s %s\n" CC $$<
	    @$$(CC) $$(CFLAGS) $$< -o $$@ -D LIB_DYNAMIC -D LIB_DYLIB=\"liblib_shared$(o).so\" -O$(o)

)
endef

$(eval $(call objs))

executables: $(execs)

# =============================================================================

run%: zad%
	@printf "%10s %s\n" RUN $<
	LD_LIBRARY_PATH=./obj/ ./obj/zad$(@:run%=%).$(VER).o$(O) $(ARGS)

gdb%: zad%
	@printf "%10s %s\n" GDB $<
	LD_LIBRARY_PATH=./obj/ $(GDB) ./obj/zad$(@:gdb%=%).$(VER).o$(O) $(ARGS)

valgrind%: zad%
	@printf "%10s %s\n" VALGRIND $<
	LD_LIBRARY_PATH=./obj/ valgrind ./obj/zad$(@:valgrind%=%).$(VER).o$(O) $(ARGS)


define results

.PHONY: results3a.txt
results3a.txt: obj/zad2.static.o3 obj/zad2.shared.o3 obj/zad2.dynamic.o3
	@echo > results3a.txt

	$(foreach ver, static shared dynamic,
	    echo $(ver) -O$(o) >> results3a.txt
	    LD_LIBRARY_PATH=./obj/ ./obj/zad2.$(ver).o3 `cat ./cmd.txt | xargs` >> results3a.txt
	    @echo >> results3a.txt
	    @echo >> results3a.txt
	    @echo >> results3a.txt
	)

.PHONY: results3b.txt
results3b.txt: zad2.static zad2.shared zad2.dynamic 
	@echo > results3b.txt

	$(foreach o, $(opts),
	    $(foreach ver, static shared dynamic,
		echo $(ver) >> results3b.txt
		LD_LIBRARY_PATH=./obj/ ./obj/zad2.$(ver).o$(o) `cat ./cmd.txt | xargs` >> results3b.txt
		@echo >> results3b.txt
		@echo >> results3b.txt
		@echo >> results3b.txt
	    )
	)
endef

$(eval $(call results))

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

$(ARCHIVE_TOP)/%:
	@printf "%10s %s\n" INSTALL $(@:$(ARCHIVE_TOP)/%=%)
	@install -C -D $(@:$(ARCHIVE_TOP)/%=%) ./$@


$(ARCHIVE): $(ARCHIVE_CONTENTS)
	@printf "%10s %s\n" TAR $(ARCHIVE)
	@tar -czf $(ARCHIVE) $(AUTHOR)

	@printf "%10s %s\n" RM $(AUTHOR)
	@rm -rf $(AUTHOR)

archive: $(ARCHIVE)
