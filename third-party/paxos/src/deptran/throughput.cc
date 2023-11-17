#include "throughput.h"

namespace janus
{
    void ThroughputCalculator::add_request_end_time()
    {
        std::chrono::system_clock::time_point end_time = std::chrono::system_clock::now();
        request_end_times_l_.lock();
        request_end_times_.push_back(end_time);
        request_end_times_l_.unlock();
    };

    double janus::ThroughputCalculator::get_throughput(double time_difference)
    {
        request_end_times_l_.lock();
        double num_requests = request_end_times_.size();
        request_end_times_.clear();
        request_end_times_l_.unlock();
        Log_info("Number of requests in the last %f seconds: %f", time_difference, num_requests);
        return num_requests / time_difference;
    };

} // namespace janus