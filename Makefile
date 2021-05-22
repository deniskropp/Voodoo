CXXFLAGS = -std=c++17 -O2 -g2 -I. -I../parallel_f

all: Voodoo.o
	$(MAKE) -C VoodooTest1
	$(MAKE) -C VoodooTestGraphics
	$(MAKE) -C VoodooTestMsg

Voodoo.o: Voodoo.cpp Voodoo.h ../parallel_f/*.hpp
	$(CXX) -c -o $@ $< $(CXXFLAGS) `pkg-config --cflags sfml-network`

clean:
	rm -f Voodoo.o
	$(MAKE) -C VoodooTest1 clean
	$(MAKE) -C VoodooTestGraphics clean
	$(MAKE) -C VoodooTestMsg clean
