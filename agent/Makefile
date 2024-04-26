all: host guest

host:
	cc -static -std=c2x host.c -o $@

guest:
	cc -static -std=c2x -DHARNESS='"<<<TBD>>>"' guest.c -o $@

clean:
	-rm -f host guest
