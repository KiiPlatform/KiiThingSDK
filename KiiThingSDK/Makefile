all: build doc

doc:
	doxygen doxyfile

build_jannson:
	$(MAKE) -C jannson

build: build_jannson
	gcc -shared kii_cloud.c kii_libc.c kii_utils.c kii_logger.c -L jannson -I curl -I jannson -l jannson -l curl -o libkii.so

clean_jannson:
	$(MAKE) -C jannson clean

clean: clean_jannson
	rm -rf libkii.so

phony: doc build build_jannson clean clean_jannson
