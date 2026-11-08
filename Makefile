BIN_DIR   := $(HOME)/Library/Application Support/hyperx-fix
PLIST     := $(HOME)/Library/LaunchAgents/local.hyperx-fix.plist
LABEL     := local.hyperx-fix
DOMAIN    := gui/$(shell id -u)

hyperx-fix: hyperx-fix.c
	clang -O2 -Wall -o $@ $< -framework IOKit -framework CoreFoundation

install: hyperx-fix
	-launchctl bootout $(DOMAIN)/$(LABEL) 2>/dev/null
	mkdir -p "$(BIN_DIR)"
	cp hyperx-fix "$(BIN_DIR)/hyperx-fix"
	sed 's|__BIN__|$(BIN_DIR)/hyperx-fix|' local.hyperx-fix.plist.in > "$(PLIST)"
	launchctl bootstrap $(DOMAIN) "$(PLIST)"

uninstall:
	-launchctl bootout $(DOMAIN)/$(LABEL)
	rm -f "$(PLIST)"
	rm -rf "$(BIN_DIR)"

status:
	@launchctl print $(DOMAIN)/$(LABEL) | grep -E '^\s*(state|pid)' || true
	@cat /tmp/hyperx-fix.log 2>/dev/null || true

clean:
	rm -f hyperx-fix

.PHONY: install uninstall status clean
