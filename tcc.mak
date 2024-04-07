
CFLAGS := -DCONFIG_TCC_PREDEFS -DVM_USE_TCC $(CFLAGS)

IGNORE != cd vendor/tcc && ./configure
IGNORE != echo '""' > vendor/tcc/tccdefs_.h

GCJCIT = NO

include core.mak
