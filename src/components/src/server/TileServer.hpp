//
// Created by Nicholas Robison on 6/1/20.
//

#ifndef MOBILITY_CPP_TILESERVER_HPP
#define MOBILITY_CPP_TILESERVER_HPP

#include "../TileDimension.hpp"
#include "components/data.hpp"
#include <hpx/include/components.hpp>
#include <blaze/math/CompressedMatrix.h>

#include <cstddef>

namespace components::server {

    class TileServer : public hpx::components::component_base<TileServer> {

    public:
        typedef blaze::CompressedMatrix<int> visit_matrix;
        typedef blaze::CompressedMatrix<double> distance_matrix;

        explicit TileServer(TileDimension dim, std::string output_dir, const std::string &output_name);

        void init(const std::string &filename, std::size_t num_nodes);

        HPX_DEFINE_COMPONENT_ACTION(TileServer, init);

    private:
        components::TileDimension _dim;
        const std::string _output_dir;
        const std::string _output_name;

        void writeResults() const;
    };
}

HPX_REGISTER_ACTION_DECLARATION(::components::server::TileServer::init_action, tile_server_init_action);

#endif //MOBILITY_CPP_TILESERVER_HPP