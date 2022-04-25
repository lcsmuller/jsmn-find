PREFIX  = /usr/local/share/man/man3
MANPAGE = jsmn-find.3

DOCS_DIR = docs
LOCAL    = $(DOCS_DIR)/$(MANPAGE)
TARGETS  = $(PREFIX)/$(MANPAGE)

install: $(TARGETS)

uninstall:
	rm -rf $(TARGETS)

$(TARGETS): $(LOCAL)
	cp $< $@

.PHONY: install uninstall
