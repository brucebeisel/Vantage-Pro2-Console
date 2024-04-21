TOPTARGETS := all clean depend

SUBDIRS := \
	source/archive-dumper \
	source/archive-fixer \
	source/archive-verifier \
	source/console-archive-dumper \
	source/loop-packet-dumper \
	source/vws 

$(TOPTARGETS): $(SUBDIRS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

