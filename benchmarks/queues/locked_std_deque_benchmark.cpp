#include <wip/queues/locked_std_deque.hpp>

#include <benchmark/benchmark.h>

#include "templates/queues.hpp"
#include "templates/templates.hpp"

namespace benchmarks::queues {

namespace queues = wip::queues;
namespace utils = wip::utils;

template <typename Value>
using queue_t = queues::locked_std_deque<
    Value, std::size_t, utils::kDynamicExtent<std::size_t>,
    std::allocator<
        queues::detail::locked_std_deque_entry<Value, alignof(Value)>>>;

BENCHMARK_TEMPLATE_T(queues_locked_std_deque, nonblocking_throughput, int,
                     queue_t<int>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1023, 1048576, 2, 2})
    ->Args({1023, 1048576, 1, 4})
    ->Args({1023, 1048576, 4, 1})
    ->Args({1023, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_locked_std_deque, nonblocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1023, 1048576, 2, 2})
    ->Args({1023, 1048576, 1, 4})
    ->Args({1023, 1048576, 4, 1})
    ->Args({1023, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_locked_std_deque, blocking_throughput, int,
                     queue_t<int>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1023, 1048576, 2, 2})
    ->Args({1023, 1048576, 1, 4})
    ->Args({1023, 1048576, 4, 1})
    ->Args({1023, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

BENCHMARK_TEMPLATE_T(queues_locked_std_deque, blocking_throughput,
                     shared_ptr_int, queue_t<std::shared_ptr<int>>)
    ->Args({1023, 1048576, 1, 1})  ///< capacity, items, producers, consumers
    ->Args({1023, 1048576, 2, 2})
    ->Args({1023, 1048576, 1, 4})
    ->Args({1023, 1048576, 4, 1})
    ->Args({1023, 1048576, 4, 4})
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace benchmarks::queues
