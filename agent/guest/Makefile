all: guest

guest:
	cc -static -std=c2x -DHARNESS='"<<<TBD>>>"' guest.c -o $@

clean:
	-rm -f guest
