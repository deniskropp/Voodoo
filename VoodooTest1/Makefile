CXXFLAGS = -std=c++17 -O2 -g2 -pthread -I.. -I../../parallel_f

all: VoodooTest1

VoodooTest1: VoodooTest1.cpp ../Voodoo.o ../../parallel_f/*.hpp
	$(CXX) -o $@ $< $(CXXFLAGS) ../Voodoo.o `pkg-config --cflags --libs sfml-network`


clean:
	rm -f VoodooTest1
