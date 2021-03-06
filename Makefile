#
#	This is a sample make file for RTP

CFLAGS = -Wall -Wextra -std=gnu99 -O2 -g
AFLAGS = -D LINUX
SUBMIT = *.c Makefile *.h prj5-server.py answer.txt

client: client.c rtp.o $(NETWORK_OBJECT)
	gcc $(CFLAGS) -o prj5-client client.c rtp.o
rtp.o : rtp.c rtp.h
	gcc -c $(CFLAGS) $(AFLAGS) rtp.c

submit:	clean
	tar cvfz prj5-submit.tar.gz $(SUBMIT)

clean:
	rm -rf rtp.o prj5-client
