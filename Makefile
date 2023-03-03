TOPTARGETS := all clean depend

SUBDIRS := source/vws source/archive-dumper source/console-archive-dumper source/loop-packet-dumper

$(TOPTARGETS): $(SUBDIRS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

