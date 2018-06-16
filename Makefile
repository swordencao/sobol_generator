MPICXX=mpic++
CXX_FLAGS=-std=gnu14 -O2 -march=native -pedantic -Wall -Wextra -Wno-unused

all: example

example: example.cpp sobol_generator/sobol_generator.hpp sobol_generator/type_to_mpi.hpp
	$(MPICXX) $(CXX_FLAGS) -o example example.cpp
	
