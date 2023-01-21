SDIR=src
IDIR=include
BDIR=build
ODIR=$(BDIR)/obj
LDIR=lib

CC=gcc
CFLAGS=-I$(IDIR)

LIBS=""			# Add libraries here

# Add include files here
_DEPS = socketManager.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

# Add object files here (src files but with .o)
_OBJ = ftpClient.o socketManager.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ftpClient: $(OBJ)
	$(CC) -o $(BDIR)/$@ $^ $(CFLAGS)

.PHONY: clean

run:
	make ftpClient
	./build/ftpClient

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~

