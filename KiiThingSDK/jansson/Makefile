all=build

build:
	gcc -D int32_t=__int32_t -include stdint.h -shared -std=gnu99 dump.c error.c hashtable.c hashtable_seed.c load.c memory.c pack_unpack.c strbuffer.c strconv.c utf.c value.c -o libjannson.so

clean:
	rm -rf libjannson.so

phony: build clean
