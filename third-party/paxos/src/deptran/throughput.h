#include "__dep__.h"

namespace janus
{
    class ThroughputCalculator
    {
    public:
        // Constructor
        ThroughputCalculator() = delete;
        // variables
        vector<std::chrono::system_clock::time_point> request_end_times_;
        // functions
        void add_request_end_time();
        double get_throughput(double time_difference);
    };
} // namespace janus