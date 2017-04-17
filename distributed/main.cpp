#include <iostream>
#include <sstream>

#include <mpi.h>

#include <vector>
#include <limits>
#include <random>

#define PPMT_MAX_DIM 30
#include "PrimitivePolynomialsModuloTwo/PrimitivePolynomialsModuloTwoUpToDegree27.h"

#include "type_to_mpi.h"


template <std::size_t num_dims = PPMT_MAX_DIM,
          typename RealType = double>
class sobol_generator {
public:
    using x_t = unsigned long;
    static constexpr auto num_bits = std::numeric_limits<x_t>::digits;
    static constexpr RealType norm = 1 / (static_cast<RealType>(1) + std::numeric_limits<x_t>::max());

    decltype(new x_t[num_dims][num_bits]) direction_integers;
    decltype(new x_t[num_dims][num_bits]) leap_integers;

    int frog_leap_len;

    unsigned long count;
    std::vector<x_t> x;

    template <typename URNG>
    void initialize_direction_integers(URNG && gen) {
        std::uniform_int_distribution<x_t> dis;
        // k=0
            for(int l=1; l<=num_bits; ++l) {
                x_t v = static_cast<x_t>(1) << (num_bits - l);
                direction_integers[0][l-1] = v;
            }
        for(std::size_t k=1, pi=0, degree=1; k<num_dims; ++k, ++pi) {
            auto polynomial = PrimitivePolynomials[degree-1][pi];
            if(polynomial < 0) polynomial = PrimitivePolynomials[degree++][pi=0];

            direction_integers[k][0] = static_cast<x_t>(1) << (num_bits - 1);
            for(std::size_t l=2; l<=degree; ++l) {
                x_t v = ((dis(gen) << 1) | 1) << (num_bits - l);
                direction_integers[k][l-1] = v;
            }
            for(std::size_t l=degree+1; l<=num_bits; ++l) {
                x_t v = direction_integers[k][l-degree-1] >> degree;
                v ^= direction_integers[k][l-degree-1];
                auto p = polynomial;
                for(std::size_t j=degree-1; p; --j, p >>= 1) {
                    if(p & 1) v ^= direction_integers[k][l-j-1];
                }
                direction_integers[k][l-1] = v;
            }
        }
    }

    void initialize_leap_integers(int num_processes) {
        for(std::size_t k=0; k<num_dims; ++k) {
            for(std::size_t l=__builtin_ctz(num_processes); l<=num_bits; ++l) {
                x_t v = 0;
                for(int i=0; i<num_processes; ++i) {
                    auto j = __builtin_ctzl((1ul<<(l-1))-i);  //TODO tuki sm dal -1 pa dela, zakaj??
                    v ^= direction_integers[k][j];
                }
                leap_integers[k][l-1] = v;
            }
        }
    }


    void increment() {
        ++count;
        int j = __builtin_ctzl(count);
        for(std::size_t k=0; k<num_dims; ++k)
            x[k] ^= direction_integers[k][j];
    }

    void frog_leap() {
        count += frog_leap_len;
        int j = __builtin_ctzl(count);
        if((1<<j) < frog_leap_len) std::cout << "AAAA" << std::endl;
        for(std::size_t k=0; k<num_dims; ++k)
            x[k] ^= leap_integers[k][j];
    }

public:

    template <typename URNG>
    sobol_generator(URNG && gen, MPI_Comm mpi_comm)
      : direction_integers(new x_t[num_dims][num_bits])
      , leap_integers(new x_t[num_dims][num_bits])
      , count(0)
      , x(num_dims) {
        
        int mpi_comm_size;
        MPI_Comm_size(mpi_comm, &mpi_comm_size);

        frog_leap_len = mpi_comm_size;

        int mpi_comm_rank;
        MPI_Comm_rank(mpi_comm, &mpi_comm_rank);

        if(mpi_comm_rank == 0) {
            initialize_direction_integers(gen);
        }
        MPI_Bcast(direction_integers, num_dims*num_bits, type_to_mpi<x_t>::datatype, 0, mpi_comm);

        initialize_leap_integers(mpi_comm_size);

        for(int i=0; i<mpi_comm_rank; ++i) {
            increment();
        }
        count = 0;
    }

    ~sobol_generator() {
        delete[] direction_integers;
    }

    std::vector<RealType> operator()() {
        std::vector<RealType> y(begin(x),end(x));
        for(auto & y_k : y) y_k *= norm;
        frog_leap();
        return y;
    }

    template <typename ForwardIt>
    void generate(ForwardIt first) {
        for(auto x_k : x) *(first++) = x_k * norm;
        frog_leap();
    }
};

using namespace std;

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    random_device rd;
    default_random_engine gen(1234);

    sobol_generator<3> sg(gen, MPI_COMM_WORLD);

    double y[3];
    for(int i=0; i<5; ++i) {
        sg.generate(y);
        ostringstream oss;
        oss << i << rank << ": " << y[1] << " " << y[2] << endl;
        cout << oss.str();
    }

    MPI_Finalize();
}
