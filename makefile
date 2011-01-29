
CXX=g++
CXXFLAGS= -c -v -Wall -mwindows

targetdir=bin
srcdir=src
smpldir=samples

all: $(targetdir)/interprocess.o $(targetdir)/client.exe $(targetdir)/host.exe
interprocess: $(targetdir)/interprocess.o



$(targetdir)/client.exe: $(targetdir)/interprocess.o client.o
	$(CXX) client.o $(targetdir)/interprocess.o -o $(targetdir)/client.exe
	

client.o:$(smpldir)/client.c $(srcdir)/interprocess.h
	$(CXX) $(CXXFLAGS) $(smpldir)/client.c
	


$(targetdir)/host.exe: $(targetdir)/interprocess.o host.o
	$(CXX) host.o $(targetdir)/interprocess.o -o $(targetdir)/host.exe
	

host.o:$(smpldir)/host.c $(srcdir)/interprocess.h
	$(CXX) $(CXXFLAGS) $(smpldir)/host.c 
	
$(targetdir)/interprocess.o: $(srcdir)/interprocess.h $(srcdir)/interprocess.c
	$(CXX) $(CXXFLAGS) $(srcdir)/interprocess.c -o $(targetdir)/interprocess.o




.PHONY: run
run:
	start $(targetdir)/host.exe 
	$(targetdir)/client.exe
	
	
.PHONY: clean
clean:	
	rm -rfv *.o 
	rm -rfv bin/*.exe bin/*.o
