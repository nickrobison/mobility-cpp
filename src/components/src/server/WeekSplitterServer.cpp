//
// Created by Nicholas Robison on 5/22/20.
//

#include "WeekSplitterServer.hpp"

#include "spdlog/spdlog.h"
#include <arrow/api.h>
#include <hpx/parallel/execution.hpp>
#include <hpx/parallel/algorithms/transform_reduce.hpp>
#include <utility>

using ArrowDate = arrow::Date32Type::c_type;

using namespace std;
namespace par = hpx::parallel;

namespace components::server {
    WeekSplitter::WeekSplitter(vector<string> filenames) : files(move(filenames)) {}

    vector<visit_row> WeekSplitter::invoke() const {
        spdlog::debug("Invoking week splitter on {}", hpx::get_locality_id());
        return par::transform_reduce(
                par::execution::par_unseq,
                files.begin(),
                files.end(),
                vector<visit_row>(),
                [](vector<visit_row> acc, vector<visit_row> v) {
                    acc.reserve(acc.size() + v.size());
                    move(v.begin(), v.end(), back_inserter(acc));
                    return acc;
                }, [](const auto f) {
                    return WeekSplitter::handleFile(f);
                });
    }

    vector<visit_row> WeekSplitter::handleFile(string const &filename) {
        spdlog::debug("Reading file: {}", filename);
        const auto parquet = io::Parquet(filename);
        const auto table = parquet.read();
        const auto rows = WeekSplitter::tableToVector(table);

        spdlog::debug("Beginning expand {} rows to {}", rows.size(), rows.size() * 7);
        return par::transform_reduce(
                par::execution::seq,
                rows.begin(),
                rows.end(),
                vector<visit_row>(),
                [](vector<visit_row> acc, vector<visit_row> v) {
                    acc.reserve(acc.size() + v.size());
                    move(v.begin(), v.end(), back_inserter(acc));
                    return acc;
                },
                [](data_row row) {
                    vector<visit_row> out;
                    out.reserve(row.visits.size());
                    for (int i = 0; i < row.visits.size(); i++) {
                        auto visit = row.visits[i];
                        out.push_back({row.location_cbg, row.visit_cbg, row.date + date::days{i}, visit, row.distance,
                                       visit * row.distance});
                    }
                    return out;
                });
    }

    vector<data_row> WeekSplitter::tableToVector(shared_ptr<arrow::Table> const table) {
        vector<data_row> rows;
        rows.reserve(table->num_rows());

        auto location_cbg = static_pointer_cast<arrow::StringArray>(table->column(4)->chunk(0));
        auto visit_cbg = static_pointer_cast<arrow::StringArray>(table->column(0)->chunk(0));
        auto date = static_pointer_cast<arrow::Date32Array>(table->column(1)->chunk(0));
        auto distance = static_pointer_cast<arrow::DoubleArray>(table->column(10)->chunk(0));
        auto visits = static_pointer_cast<arrow::StringArray>(table->column(2)->chunk(0));

        for (int64_t i = 0; i < table->num_rows(); i++) {
            const string cbg = location_cbg->GetString(i);
            const string visit = visit_cbg->GetString(i);
            const ArrowDate d2 = date->Value(i);
            const double d = distance->Value(i);
            const string visit_str = visits->GetString(i);
            vector<uint32_t> v2;
            try {
                v2 = WeekSplitter::split(visit_str, ',');
            }
            catch (const invalid_argument &e) {
                spdlog::critical("Problem doing conversion: {}\n{}", e.what(), visit_str);
            }

            data_row v{cbg, visit, date::sys_days(date::days{d2}), v2, d};

            rows.push_back(v);
        }

        return rows;
    }

    vector<uint32_t> WeekSplitter::split(string const &str, char delim) {
        vector<uint32_t> strings;
        size_t start;
        size_t end = 0;
        while ((start = str.find_first_not_of(delim, end)) != string::npos) {
            end = str.find(delim, start);
            auto base = str.substr(start, end - start);
            base.erase(remove_if(base.begin(), base.end(), &WeekSplitter::IsParenthesesOrDash), base.end());
            strings.push_back(stoi(base));
        }
        return strings;
    }

    bool WeekSplitter::IsParenthesesOrDash(char c) {
        switch (c) {
            case '[':
            case ']':
            case ' ':
            case '-':
                return true;
            default:
                return false;
        }
    }
}