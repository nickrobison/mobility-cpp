//
// Created by Nicholas Robison on 6/4/20.
//

#ifndef MOBILITY_CPP_TEMPORALMATRICIES_HPP
#define MOBILITY_CPP_TEMPORALMATRICIES_HPP

#include <utility>
#include <absl/synchronization/mutex.h>
#include <blaze/math/CompressedMatrix.h>

typedef blaze::CompressedMatrix<uint32_t, blaze::columnMajor> visit_matrix;
typedef blaze::CompressedMatrix<double, blaze::columnMajor> distance_matrix;

namespace components {

    struct MatrixPair {
        visit_matrix vm;
        distance_matrix dm;
        MatrixPair(visit_matrix vm, distance_matrix dm) : vm(std::move(vm)),
                                                          dm(std::move(dm)) {}
    };


    class TemporalMatricies {

    public:
        TemporalMatricies(size_t matricies, size_t x_dimension, size_t y_dimension);

        void insert(std::size_t time, std::size_t x, std::size_t y, std::uint16_t visits, double distance);

        distance_matrix compute(std::size_t i = 0);

    private:
        std::vector<MatrixPair> matricies;
        std::vector<absl::Mutex> _locks;
    };
}


#endif //MOBILITY_CPP_TEMPORALMATRICIES_HPP
