RM=rm
CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=fasm.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=fasm

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) -f $(OBJECTS)
	$(RM) -f $(EXECUTABLE)
	$(RM) -f $(EXECUTABLE).exe