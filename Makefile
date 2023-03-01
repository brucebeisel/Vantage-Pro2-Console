all: vws archive-dumper console-archive-dumper loop-dumper

vws: source/vws/vws
	cd source/vws; make all; cd ../..

archive-dumper: source/archive-dumper/archive-dumper
	cd source/archive-dumper; make all; cd ../..

console-archive-dumper: source/console-archive-dumper/console-archive-dumper
	cd source/console-archive-dumper; make all; cd ../..

loop-dumper: source/loop-packet-dumper/loop-dumper
	cd source/loop-packet-dumper; make all; cd ../..
