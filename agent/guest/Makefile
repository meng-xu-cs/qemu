all: guest init

guest:
	cc -static -std=c2x -DMODE_Test=1 -DMODE_Fuzz=1 -DMODE_Shell=1 -DVIRTME=1 guest.c -o $@

init:
	cc -static -std=c2x -DMODE_Test=1 -DMODE_Fuzz=1 -DMODE_Shell=1 -DVIRTME=1 init-test.c -o $@

clean:
	-rm -f guest init
