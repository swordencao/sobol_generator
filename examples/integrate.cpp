#include <cstdio>
#include <random>
#include <vector>
#include <cmath>

#include <mpi.h>

#define PPMT_MAX_DIM 30
#include "../sobol_generator/sobol_generator.hpp"

using namespace std;

double f(double x, double y) {
    return sin(x) * pow(y,2);
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    int comm_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

    random_device rd;
    default_random_engine gen(rd());

    sobol::sobol_generator<3> sg(gen, MPI_COMM_WORLD);
    double y[3];

    int n = 1000000;

    double sum = 0;
    for(int i=0; i<n; ++i) {
        sg.generate(y);
        sum += f(y[1],y[2]);
    }

    if(comm_rank == 0) {
        MPI_Reduce(MPI_IN_PLACE,&sum,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
        printf("%.16lf\n", sum/(comm_size*n));
    } else {
        MPI_Reduce(&sum,&sum,1,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
    }

    MPI_Finalize();
}