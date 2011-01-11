
CXX = g++
CXXFLAGS= -c -v -Wall -mwindows

targetdir=bin
srcdir=src

all: $(srcdir)/interprocess.o $(targetdir)/client.exe
interprocess: $(targetdir)/interprocess.o



$(targetdir)/client.exe: $(targetdir)/interprocess.o $(srcdir)/client.o
	$(CXX) $(srcdir)/client.o $(targetdir)/interprocess.o -o $(targetdir)/client.exe
	

$(srcdir)/client.o:$(srcdir)/client.c $(srcdir)/interprocess.h
	$(CXX) $(CXXFLAGS) $(srcdir)/client.c 
	


$(targetdir)/interprocess.o : $(srcdir)/interprocess.h $(srcdir)/interprocess.c
		$(CXX) $(CXXFLAGS) $(srcdir)/interprocess.c -o $(targetdir)/interprocess.o




.PHONY: run
run:
	bin/client.exe
	
.PHONY: clean
clean:	
	rm -rf *.o 