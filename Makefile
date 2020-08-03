CPPFLAGS=-I../install/include
LDFLAGS=-L../install/lib
CFLAGS=-Wall -std=c11

all: exegetical.so

exegetical.so: exegetical.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(CFLAGS) -shared -D BUILD_MODULE -fPIC -o exegetical.so exegetical.c -lflux-core

clean:
	rm exegetical.so
