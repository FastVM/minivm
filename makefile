
CC ?= cc

NO_CROSSJUMPING := ${shell ${CC} -fno-crossjumping -E -x c /dev/null -o /dev/null 2>/dev/null && echo -fno-crossjumping}
ALIGN_LABELS := ${shell ${CC} -Werror -falign-labels=16 -c -x c /dev/null -o /dev/null 2>/dev/null && echo -falign-labels=16}

OPT ?= -O3 -fno-math-errno -fno-trapping-math -fno-stack-protector ${NO_CROSSJUMPING} ${ALIGN_LABELS}

DEFINES ?= -DNDEBUG

EXE ?=


LIBM ?= YES
LIBM_NO :=
LIBM_YES := -lm
LIBM_FLAGS := ${LIBM_${LIBM}}

EXPORT_DYNAMIC ?= -rdynamic
LIBDL ?= -ldl

PREFIX ?= /usr/local
DESTDIR ?=

BASE_CFLAGS := ${OPT} ${DEFINES} ${CFLAGS}
BASE_LDFLAGS := ${OPT} ${LDFLAGS} ${LIBM_FLAGS} ${LIBDL} ${EXPORT_DYNAMIC}

CMODS := build/cmod/luamod.so build/cmod/luajson.so build/cmod/luaabi.so build/cmod/mxmod.so
CMOD_LDFLAGS ?=

SRCS := ${shell find src -name '*.c'}
IC_SRCS := vendor/isocline/src/isocline.c
MAIN_SRCS = ${SRCS} ${IC_SRCS}
MAIN_OBJS = ${MAIN_SRCS:%.c=build/obj/%.o}

MAIN_DEPS = ${MAIN_SRCS:%.c=build/dep/%.dep}

LIB_OBJS = ${filter-out build/obj/src/cli/% build/obj/vendor/%,${MAIN_OBJS}}

default: all

all: minivm

minivm: build/bin/minivm${EXE}

cmod: ${CMODS}

lib: build/lib/libminivm.a

embed: build/embed/host${EXE}

clean: .dummy
	rm -rf build

install: build/bin/minivm .dummy
	install -d ${DESTDIR}${PREFIX}/bin ${DESTDIR}${PREFIX}/include/minivm
	install -m 755 build/bin/minivm ${DESTDIR}${PREFIX}/bin/minivm
	install -m 644 include/lua.h include/lauxlib.h include/lualib.h include/luaconf.h ${DESTDIR}${PREFIX}/include/minivm

build/cmod/%.so: test/cmod/%.c include/lua.h include/lauxlib.h include/luaconf.h
	@mkdir -p ${dir ${@}}
	${CC} -shared -fPIC -Iinclude ${OPT} -o ${@} ${<} ${CMOD_LDFLAGS}

build/lib/libminivm.a: ${LIB_OBJS}
	@mkdir -p ${dir ${@}}
	${AR} rcs ${@} ${LIB_OBJS}

build/embed/host${EXE}: test/embed/host.c build/lib/libminivm.a include/lua.h include/lauxlib.h include/lualib.h include/luaconf.h
	@mkdir -p ${dir ${@}}
	${CC} -Iinclude ${OPT} -o ${@} ${<} build/lib/libminivm.a ${LIBM_FLAGS} ${LIBDL}

build/bin/minivm${EXE}: ${MAIN_OBJS}
	@mkdir -p ${dir ${@}}
	${CC} -o ${@} ${MAIN_OBJS} ${BASE_LDFLAGS}

${MAIN_OBJS}:
	@mkdir -p ${dir ${@}}
	@mkdir -p ${dir ${@:build/obj/%.o=build/dep/%.dep}}
	${CC} ${@:build/obj/%.o=%.c} -c -o ${@} ${BASE_CFLAGS}
	@${CC} ${@:build/obj/%.o=%.c} -MM -MP -MF ${@:build/obj/%.o=build/dep/%.dep} -MT ${@} ${BASE_CFLAGS}

include ${wildcard ${MAIN_DEPS}}

.dummy:
