CXXFLAGS = -std=c++17 -pthread -I..

all: VoodooTest1

VoodooTest1: VoodooTest1.cpp ../Voodoo.o
	g++ -oVoodooTest1 VoodooTest1.cpp $(CXXFLAGS) ../Voodoo.o `pkg-config --libs sfml-network`

