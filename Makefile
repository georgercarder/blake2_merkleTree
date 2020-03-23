CXX ?= g++-5

VECTORFLAGS ?= -march=native -O3
LIBFLAGS= -lpthread
CXXFLAGS ?= -std=c++1y 
DEBUGFLAGS= #-g

INCLUDEDIRS ?= -I./hash_functions

OBJECTS= blake2s.o test.o 

merkleTree: merkle.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LIBFLAGS) $(OBJECTS) merkle.cpp $(VECTORFLAGS) $(DEBUGFLAGS) 


blake2s.o: hash_functions/blake2s.c
	$(CXX) $(INCLUDEDIRS) -c hash_functions/blake2s.c $(VECTORFLAGS)

test.o: test.cpp
	$(CXX) -c test.cpp $(VECTORFLAGS)
