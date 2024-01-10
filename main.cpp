#include <iostream>

#include <marl/defer.h>
#include <marl/event.h>
#include <marl/scheduler.h>
#include <marl/waitgroup.h>
#include <marl/ticket.h>

#include <vector>
#include <chrono>

void process_chunk(std::vector<int64_t>& data, const size_t start, const size_t end) {
    for (size_t i = start; i < end; ++i) {
      for (int spin = 0; spin < 10000; ++spin) {
        data[i] *= 2;
      }
    }
}

void without_tasks() {
  std::vector<int64_t> numbers(10'000, 1);
  process_chunk(numbers, 0, numbers.size());
  // std::copy(
  //   numbers.begin(), numbers.end(), std::ostream_iterator<int64_t>(std::cout));
}

void with_tasks() {
  std::vector<int64_t> numbers(10'000, 1);
  marl::WaitGroup wait_group;
  const size_t chunk_size = numbers.size() / marl::Thread::numLogicalCPUs();
  for (size_t i = 0; i < numbers.size(); i += chunk_size) {
    size_t start = i;
    size_t end = std::min(start + chunk_size, numbers.size());
    wait_group.add(1);
    marl::schedule([=, &numbers, &wait_group] {
      defer(wait_group.done());
      process_chunk(numbers, start, end);
    });
  }
  wait_group.wait();
  // std::copy(
  //   numbers.begin(), numbers.end(), std::ostream_iterator<int64_t>(std::cout));
}

int main(int argc, char* argv[]) {
  marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
  scheduler.bind();
  defer(scheduler.unbind());

  size_t with_tasks_before = 0;
  {
    auto before = std::chrono::steady_clock::now();
    with_tasks();
    auto after= std::chrono::steady_clock::now();
    with_tasks_before = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
    std::cout << "with_tasks - Time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() << " ms\n";
  }

  size_t without_tasks_before = 0;
  {
    auto before = std::chrono::steady_clock::now();
    without_tasks();
    auto after= std::chrono::steady_clock::now();
    without_tasks_before = std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();
    std::cout << "without_tasks - Time: " << without_tasks_before << " ms\n";
  }

  std::cout << "speed up: " << ((double)without_tasks_before / (double)with_tasks_before) << '\n';

  return 0;
}
