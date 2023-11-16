#include "throughput.h"

namespace janus
{
    void ThroughputCalculator::add_request_end_time()
    {
        std::chrono::system_clock::time_point end_time = std::chrono::system_clock::now();
        auto it = std::lower_bound(request_end_times_.begin(), request_end_times_.end(), end_time);
        request_end_times_.insert(it, end_time);
    };

    double janus::ThroughputCalculator::get_throughput(double time_difference)
    {
        double num_requests = 0;
        for (auto it = request_end_times_.begin(); it != request_end_times_.end(); ++it)
        {
            if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - *it).count() < time_difference)
            {
                ++num_requests;
            }
        }
        request_end_times_.clear();
        return num_requests / time_difference;
    };

} // namespace janus