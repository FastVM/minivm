
default: all

all: compile_commands.json clang-format info.txt

info.txt: .dummy
	$(MAKE) -Bj1 -f core.mak OPT='-fanalyzer -fmax-errors=0' 2>&1 2> $(@)

compile_commands.json: .dummy
	bear -- $(MAKE) -Bj -f core.mak

clang-format: .dummy
	clang-format -i $$(find vm main -name '*.c' -or -name '*.h')

.dummy:
