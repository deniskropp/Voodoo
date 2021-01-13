CXXFLAGS = -std=c++17

all: Voodoo.o
	make -C VoodooTest1
	make -C VoodooTestGraphics

Voodoo.o: Voodoo.cpp Voodoo.h
	g++ -c -oVoodoo.o Voodoo.cpp $(CXXFLAGS) `pkg-config --libs sfml-network`
