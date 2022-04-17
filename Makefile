CXXFLAGS=-g -std=c++11
CXX=g++

mmu: simulation.cpp process.cpp process.hpp pager.cpp pager.hpp vma.cpp vma.hpp pte.hpp utils.cpp utils.hpp
	$(CXX) $(CXXFLAGS) simulation.cpp process.cpp pager.cpp utils.cpp vma.cpp -o mmu

clean:
	rm -rf *.o
	rm -f mmu