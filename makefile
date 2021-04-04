OUT=build/nanovm

cross: build cosmopolitan-amalgamation-0.3.zip
	unzip -o cosmopolitan-amalgamation-0.3.zip
	gcc -g -Ofast -ffast-math -static -nostdlib -nostdinc -fno-pie -no-pie -mno-red-zone \
		-fno-omit-frame-pointer -pg -mnop-mcount \
		-o build/nanovm.com.dbg src/nanovm.c -fuse-ld=bfd -Wl,-T,ape.lds \
		-I. crt.o ape.o cosmopolitan.a -D__COSMO__ $(CFLAGS)
	objcopy -S -O binary build/nanovm.com.dbg $(OUT).com

unopt: build
	$(CC) src/nanovm.c -o $(OUT).exe $(CFLAGS) -lgc

opt: build 
	$(CC) src/nanovm.c -o $(OUT).exe -Ofast $(CFLAGS) -lgc

build: 
	mkdir build

cosmopolitan-amalgamation-0.3.zip:
	wget https://justine.lol/cosmopolitan/cosmopolitan-amalgamation-0.3.zip

cleans: clean
	: rm cosmopolitan-amalgamation-0.3.zip

clean: dummy
	: rm ape.lds ape.o cosmopolitan.a cosmopolitan.h crt.o -r build

dummy: