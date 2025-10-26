#include <sandbox/queues/single_threaded_queue.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = sandbox::queues;
namespace utils = sandbox::utils;

template <typename Value>
using queue_t = queues::single_threaded_queue<
    Value, std::size_t, utils::kDynamicExtent<std::size_t>,
    std::allocator<
        queues::detail::single_threaded_queue_entry<Value, alignof(Value)>>>;

BENCHMARK_TEMPLATE_T(queues_single_threaded_queue_entry,
                     single_threaded_throughput, int, queue_t<int>)
    ->Args({1024, 1048576})  ///< capacity, items
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_single_threaded_queue_entry,
                     single_threaded_throughput, shared_ptr_int,
                     queue_t<std::shared_ptr<int>>)
    ->Args({1024, 1048576})  ///< capacity, items
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
