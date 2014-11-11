all: build doc

doc:
	doxygen doxyfile

build_jansson:
	$(MAKE) -C jansson

build: build_jansson
	gcc -shared kii_cloud.c kii_custom.c kii_utils.c -L jansson -I curl -I jansson -l jansson -l curl -o libkii.so

clean_jansson:
	$(MAKE) -C jansson clean

clean: clean_jansson
	rm -rf libkii.so

phony: doc build build_jansson clean clean_jansson
