RM=rm
CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=fsim.c uart.c
HEADERS=uart.h
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=fsim
 
all: $(SOURCES) $(EXECUTABLE)
 
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
    
$(OBJECTS): $(HEADERS)
    
.c.o:
	$(CC) $(CFLAGS) $< -o $@
 
clean:
	$(RM) -f $(OBJECTS)
	$(RM) -f $(EXECUTABLE)
	$(RM) -f $(EXECUTABLE).exe
