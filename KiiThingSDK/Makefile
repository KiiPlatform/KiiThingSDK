all: build doc

all_with_openssl: build_with_openssl doc

doc:
	doxygen doxyfile

build_jansson:
	$(MAKE) -C jansson

build: build_jansson
	gcc -shared -fPIC kii_cloud.c kii_custom.c kii_prv_utils.c kii_prv_http_execute_curl.c -L jansson -I curl -I jansson -l jansson -l curl -o libkii.so

build_with_openssl: build_jansson
	gcc -shared -fPIC kii_cloud.c kii_custom.c kii_prv_utils.c kii_prv_http_execute_ssl.c -L jansson -I jansson -l jansson -l ssl -lcrypto -o libkii.so

ARCHIVE_FILES = kii_cloud.h kii_cloud.c kii_custom.h kii_custom.c kii_prv_utils.h kii_prv_utils.c kii_prv_types.h kii_http_adapter.h kii_prv_http_execute_curl.c kii_prv_http_execute_ssl.c
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
