
CXX = g++
CXXFLAGS= -c -v -Wall -mwindows

targetdir=bin
srcdir=src

all: $(srcdir)/interprocess.o $(targetdir)/client.exe $(targetdir)/host.exe
interprocess: $(targetdir)/interprocess.o



$(targetdir)/client.exe: $(targetdir)/interprocess.o $(srcdir)/client.o
	$(CXX) $(srcdir)/client.o $(targetdir)/interprocess.o -o $(targetdir)/client.exe
	

$(srcdir)/client.o:$(srcdir)/client.c $(srcdir)/interprocess.h
	$(CXX) $(CXXFLAGS) $(srcdir)/client.c
	


$(targetdir)/host.exe: $(targetdir)/interprocess.o $(srcdir)/host.o
	$(CXX) $(srcdir)/host.o $(targetdir)/interprocess.o -o $(targetdir)/host.exe
	

$(srcdir)/host.o:$(srcdir)/host.c $(srcdir)/interprocess.h
	$(CXX) $(CXXFLAGS) $(srcdir)/host.c 
	




$(targetdir)/interprocess.o : $(srcdir)/interprocess.h $(srcdir)/interprocess.c
		$(CXX) $(CXXFLAGS) $(srcdir)/interprocess.c -o $(targetdir)/interprocess.o




.PHONY: run
run:
	bin/client.exe
	
.PHONY: clean
clean:	
	rm -rf *.o 
