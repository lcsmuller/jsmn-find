PREFIX=/usr/local/share/man/man3
TARGETS=$(PREFIX)/jsmn-find.3

install: $(TARGETS)

uninstall:
	rm -rf $(TARGETS)

$(PREFIX)/jsmn-find.3:
	cp doc/jsmn-find.3 $(PREFIX)/jsmn-find.3

.PHONY: install uninstall
