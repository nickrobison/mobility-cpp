//
// Created by Nicholas Robison on 5/19/20.
//

#ifndef MOBILITY_CPP_WEEKSPLITTER_HPP
#define MOBILITY_CPP_WEEKSPLITTER_HPP

// This needs to come before the hpx includes, in order for the serialization to work.
#include "parquet.hpp"
#include "data.hpp"

#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/serialization.hpp>

namespace components {
    namespace server {
        class WeekSplitter : public hpx::components::component_base<WeekSplitter> {

        public:
            explicit WeekSplitter(std::vector<std::string> filenames);

            std::vector<visit_row> invoke() const;

            HPX_DEFINE_COMPONENT_ACTION(WeekSplitter, invoke);

        private:
            // Serialization support: even if all of the code below runs on one
            // locality only, we need to provide an (empty) implementation for the
            // serialization as all arguments passed to actions have to support this.
            friend class hpx::serialization::access;

            template<typename Archive>
            void serialize(Archive &ar, unsigned int version) const;

            static std::vector<visit_row> handleFile(std::string const &filename);

            static std::vector<data_row> tableToVector(std::shared_ptr<arrow::Table> table) ;

            static std::vector<int16_t> split(std::string const &str, char delim);

            static bool IsParenthesesOrDash(char c);

            std::vector<std::string> const files;
        };
    }

    class WeekSplitter : public hpx::components::client_base<WeekSplitter, server::WeekSplitter> {

    public:
        WeekSplitter(hpx::future<hpx::naming::id_type> &&f);

        WeekSplitter(hpx::naming::id_type &&f);

        hpx::future<std::vector<visit_row>> invoke(hpx::launch::async_policy) const;
    };
}

HPX_REGISTER_ACTION_DECLARATION(
        ::components::server::WeekSplitter::invoke_action, week_splitter_invoke_action);


#endif //MOBILITY_CPP_WEEKSPLITTER_HPP
