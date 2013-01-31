SUBDIRS = lib serverDir clientDir

default: all

app: lib

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

all clean install: $(SUBDIRS)