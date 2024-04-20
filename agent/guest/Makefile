all: guest

guest:
	cc -static -std=c2x \
		-DMODE_Test=1 -DMODE_Fuzz=1 -DMODE_Check=1 -DMODE_Shell=1 \
		-DSETUP_Virtme=1 -DSETUP_Simple=1 \
		guest.c -o $@

clean:
	-rm -f guest
