
#include <iostream>
#include <algorithm>
#include <string>

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/exception.h>
#include <range/v3/view/group_by.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/action/sort.hpp>
#include <range/v3/range/conversion.hpp>
#include <fmt/core.h>
#include <boost/filesystem.hpp>
#include <CLI/CLI.hpp>
#include "pstl/execution"
#include "pstl/algorithm"
#include "pstl/numeric"
#include "spdlog/spdlog.h"

using namespace ranges;
namespace fs = boost::filesystem;

using ArrowDate = arrow::Date32Type::c_type;

struct data_row
{
    const std::string location_cbg;
    const std::string visit_cbg;
    const ArrowDate date;
    std::vector<int16_t> visits;
    const double distance;
};

struct visit_row
{
    const std::string location_cbg;
    const std::string visit_cbg;
    const ArrowDate date;
    const int16_t visits;
    const double distance;
    const double weighted_total;
};

std::ostream &
operator<<(std::ostream &o, const data_row &dr)
{
    auto msg = fmt::format("Location: {}\n", dr.location_cbg);
    return o << msg;
}

bool IsParenthesesOrDash(char c)
{
    switch (c)
    {
    case '[':
    case ']':
    case ' ':
    case '-':
        return true;
    default:
        return false;
    }
}

std::vector<int16_t> split(const std::string &str, char delim)
{
    std::vector<int16_t> strings;
    size_t start;
    size_t end = 0;
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        auto base = str.substr(start, end - start);
        base.erase(std::remove_if(base.begin(), base.end(), &IsParenthesesOrDash), base.end());
        strings.push_back(std::stoi(base));
    }
    return strings;
}

arrow::Status
TableToVector(const std::shared_ptr<arrow::Table> &table, std::vector<struct data_row> &rows)
{

    auto location_cbg = std::static_pointer_cast<arrow::StringArray>(table->column(4)->chunk(0));
    auto visit_cbg = std::static_pointer_cast<arrow::StringArray>(table->column(0)->chunk(0));
    auto date = std::static_pointer_cast<arrow::Date32Array>(table->column(1)->chunk(0));
    auto distance = std::static_pointer_cast<arrow::DoubleArray>(table->column(10)->chunk(0));
    auto visits = std::static_pointer_cast<arrow::StringArray>(table->column(2)->chunk(0));

    for (int64_t i = 0; i < table->num_rows(); i++)
    {
        const std::string cbg = location_cbg->GetString(i);
        const std::string visit = visit_cbg->GetString(i);
        const ArrowDate d2 = date->Value(i);
        const double d = distance->Value(i);
        const std::string visit_str = visits->GetString(i);
        std::vector<int16_t> visits;
        try
        {
            visits = split(visit_str, ',');
        }
        catch (const std::invalid_argument &e)
        {
            spdlog::critical("Problem doing conversion: {}\n{}", e.what(), visit_str);
        }
        rows.push_back({cbg, visit, d2, visits, d});
    }

    return arrow::Status::OK();
}

void write_parquet_file(const arrow::Table &table, const std::string &filename)
{
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    PARQUET_ASSIGN_OR_THROW(
        outfile,
        arrow::io::FileOutputStream::Open(filename));
    // The last argument to the function call is the size of the RowGroup in
    // the parquet file. Normally you would choose this to be rather large but
    // for the example, we use a small value to have multiple RowGroups.
    PARQUET_THROW_NOT_OK(
        parquet::arrow::WriteTable(table, arrow::default_memory_pool(), outfile, 3));
}

int main(int argc, char **argv)
{
    std::string input_dir = "data/";
    std::string output_file = "./out.parquet";

    CLI::App app;
    app.add_option("input", input_dir, "Input directory");
    app.add_option("output", output_file, "Output file to write");
    CLI11_PARSE(app, argc, argv);

    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Mobilizing");

    std::vector<struct data_row> rows;

    // Ok. Let's try to read from disk

    for (auto &p : fs::directory_iterator(input_dir))
    {
        spdlog::info("Reading {}", p.path().string());
        std::shared_ptr<arrow::io::ReadableFile> infile;
        PARQUET_ASSIGN_OR_THROW(infile, arrow::io::ReadableFile::Open(p.path().string(), arrow::default_memory_pool()));

        std::unique_ptr<parquet::arrow::FileReader> reader;
        PARQUET_THROW_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(), &reader));
        reader->set_use_threads(5);

        std::shared_ptr<arrow::Table> table;
        PARQUET_THROW_NOT_OK(reader->ReadTable(&table));
        spdlog::debug("Loaded {} columns and {} rows.", table->num_columns(), table->num_rows());

        for (auto &c : table->schema()->fields())
        {
            spdlog::debug("Column {}. Type: {}", c->name(), c->type()->ToString());
        }

        spdlog::debug("Reserving an additional {} rows.", table->num_rows());
        rows.reserve(rows.size() + table->num_rows());

        auto resp = TableToVector(table, rows);
        if (!resp.ok())
        {
            spdlog::critical("Problem: {}\n", resp.ToString());
        }
    }

    spdlog::info("Rows of everything: {}\n", rows.size());

    // Now, sort and group
    auto sorted = [](auto lhs, auto rhs) {
        return lhs.location_cbg.compare(rhs.location_cbg);
    };

    auto group_location = [](const data_row &lhs, const data_row &rhs) {
        return lhs.location_cbg == rhs.location_cbg;
    };

    auto grouped = rows | views::group_by(group_location); //; // | ranges::;

    auto transformed = to_vector(grouped);

    fmt::print("I have {} groups.", transformed.size());

    // Now, expand everything
    std::vector<const visit_row> output = std::transform_reduce(
        pstl::execution::par_unseq,
        rows.begin(),
        rows.end(),
        std::vector<const visit_row>(),
        [](std::vector<const visit_row> acc, std::vector<const visit_row> v) {
            std::move(v.begin(), v.end(), std::back_inserter(acc));
            return acc;
        },
        [](const data_row &row) {
            std::vector<const visit_row> out;
            out.reserve(row.visits.size());
            for (int i = 0; i < row.visits.size(); i++)
            {
                const auto visit = row.visits[i];
                out.push_back({row.location_cbg, row.visit_cbg, row.date + 1, visit, row.distance, visit * row.distance});
            }
            return out;
        });

    spdlog::info("Expanded {} rows to {} rows.", rows.size(), output.size());

    // Write it to a new Parquet file
    // We want the cbg pair, the date, the total visits and the distance

    arrow::StringBuilder loc_cbg_builder;
    arrow::StringBuilder visit_cbg_builder;
    arrow::Date32Builder visit_date_builder;
    arrow::Int16Builder visit_count_builder;
    arrow::DoubleBuilder distance_builder;
    arrow::DoubleBuilder weight_builder;

    std::for_each(output.begin(), output.end(), [&loc_cbg_builder, &visit_cbg_builder, &visit_date_builder, &visit_count_builder, &distance_builder, &weight_builder](const visit_row &row) {
        arrow::Status status;
        status = loc_cbg_builder.Append(row.location_cbg);
        status = visit_cbg_builder.Append(row.visit_cbg);
        status = visit_date_builder.Append(row.date);
        status = visit_count_builder.Append(row.visits);
        status = distance_builder.Append(row.distance);
        status = weight_builder.Append(row.weighted_total);
    });

    arrow::Status status;

    std::shared_ptr<arrow::Array> loc_cbg_array;
    status = loc_cbg_builder.Finish(&loc_cbg_array);
    std::shared_ptr<arrow::Array> visit_cbg_array;
    status = visit_cbg_builder.Finish(&visit_cbg_array);
    std::shared_ptr<arrow::Array> visit_date_array;
    status = visit_date_builder.Finish(&visit_date_array);
    std::shared_ptr<arrow::Array> visit_count_array;
    status = visit_count_builder.Finish(&visit_count_array);
    std::shared_ptr<arrow::Array> distance_array;
    status = distance_builder.Finish(&distance_array);
    std::shared_ptr<arrow::Array> weight_array;
    status = weight_builder.Finish(&weight_array);

    auto schema = arrow::schema(
        {arrow::field("location_cbg", arrow::utf8()),
         arrow::field("visit", arrow::utf8()),
         arrow::field("visit_date", arrow::date32()),
         arrow::field("visit_count", arrow::int16()),
         arrow::field("distance", arrow::float64()),
         arrow::field("weighted_total", arrow::float64())});

    auto data_table = arrow::Table::Make(schema, {loc_cbg_array, visit_cbg_array, visit_date_array, visit_count_array, distance_array});

    write_parquet_file(*data_table, output_file);

    return 0;
}