CC = g++
CFLAGS = -W -Wall -Wextra -pedantic -O2
LDFLAGS =
LIBS = -lusb-1.0
PROJ = proskit

$(PROJ): main.o proskit.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

main.o: main.cc
	$(CC) $(CFLAGS) -c $< -o $@

proskit.o: proskit.cc
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(PROJ)
