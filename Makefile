CPPFLAGS=-I../install/include
LDFLAGS=-L../install/lib
CFLAGS=-Wall -std=c11

all: exegetical.so happiness.so

happiness.so: happiness.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(CFLAGS) -shared -D BUILD_MODULE -fPIC -o happiness.so happiness.c -lflux-core

exegetical.so: exegetical.c
	$(CC) $(CPPFLAGS) $(LDFLAGS) $(CFLAGS) -shared -D BUILD_MODULE -fPIC -o exegetical.so exegetical.c -lflux-core

clean:
	rm exegetical.so
