//
// Created by Nicholas Robison on 7/29/20.
//

#ifndef MOBILITY_CPP_SAFEGRAPHTILER_HPP
#define MOBILITY_CPP_SAFEGRAPHTILER_HPP

#include <map-tile/coordinates/Coordinate3D.hpp>
#include <components/OffsetCalculator.hpp>
#include <components/TemporalMatricies.hpp>
#include <components/ShapefileWrapper.hpp>
#include <map-tile/ctx/Context.hpp>
#include <shared/data.hpp>


class SafegraphTiler {
public:

    void setup(const mt::ctx::ReduceContext<v2, mt::coordinates::Coordinate3D> &ctx);
    void receive(const mt::ctx::ReduceContext<v2, mt::coordinates::Coordinate3D> &ctx,
                 const mt::coordinates::Coordinate3D &key,
                 const v2 &value);

    void compute(const mt::ctx::ReduceContext<v2, mt::coordinates::Coordinate3D> &ctx);

private:
    std::unique_ptr<components::detail::OffsetCalculator> _oc;
    std::unique_ptr<components::ShapefileWrapper> _s;
    std::unique_ptr<components::TemporalMatricies> _tm;
    components::TileConfiguration _tc;
};


#endif //MOBILITY_CPP_SAFEGRAPHTILER_HPP