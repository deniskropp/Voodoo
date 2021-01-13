CXXFLAGS = -std=c++17 -pthread -I..

all: VoodooTestGraphics

VoodooTestGraphics: VoodooTestGraphics.cpp ../Voodoo.o
	g++ -oVoodooTestGraphics VoodooTestGraphics.cpp $(CXXFLAGS) ../Voodoo.o `pkg-config --libs sfml-network sfml-graphics`

