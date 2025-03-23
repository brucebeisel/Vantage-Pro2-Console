TOPTARGETS := all clean depend

# vws must be first
SUBDIRS := \
	source/vws \
	source/archive-dumper \
	source/archive-fixer \
	source/archive-rebuilder \
	source/archive-verifier \
	source/command-line-console \
	source/console-archive-dumper \
	source/loop-packet-dumper \
	source/test

$(TOPTARGETS): $(SUBDIRS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

