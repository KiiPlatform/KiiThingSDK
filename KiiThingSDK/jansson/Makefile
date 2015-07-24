CC = gcc

all=build

build:
	$(CC) -D int32_t=__int32_t -include stdint.h -shared -std=gnu99 -fPIC dump.c error.c hashtable.c hashtable_seed.c load.c memory.c pack_unpack.c strbuffer.c strconv.c utf.c value.c -o libjansson.so

clean:
	rm -rf libjansson.so

phony: build clean
