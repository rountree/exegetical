CPPFLAGS=-I../variorum_install/include
LDFLAGS=-L../variorum_install/lib
CFLAGS=-Wall -std=c11

all: exegetical.so exegetical_with_json.so

exegetical.so: exegetical.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(CFLAGS) -shared -D BUILD_MODULE -fPIC -o exegetical.so exegetical.c -lflux-core

exegetical_with_json.so: exegetical_with_json.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(CFLAGS) -shared -D BUILD_MODULE -fPIC -o exegetical_with_json.so exegetical_with_json.c -lflux-core -lvariorum

clean:
	rm exegetical.so exegetical_with_json.so   
