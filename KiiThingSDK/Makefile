all: build doc

doc:
	doxygen doxyfile

build_jansson:
	$(MAKE) -C jansson

build: build_jansson
	gcc -shared -fPIC kii_cloud.c kii_custom.c kii_prv_utils.c -L jansson -I curl -I jansson -l jansson -l curl -o libkii.so

ARCHIVE_FILES = kii_cloud.h kii_cloud.c kii_custom.c kii_prv_utils.h kii_prv_utils.h kii_prv_types.h
archive: clean
	mkdir kii_sources
	cp $(ARCHIVE_FILES) kii_sources
	tar -czf libkii.tar.gz kii_sources
	rm -rf kii_sources

clean_jansson:
	$(MAKE) -C jansson clean

clean: clean_jansson
	rm -rf libkii.so
	rm -rf kii_sources

phony: doc build build_jansson clean clean_jansson
