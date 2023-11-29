#include "__dep__.h"

namespace janus
{
    struct RequestData
    {
        chrono::system_clock::time_point start_time;
        chrono::system_clock::time_point end_time;
        uint64_t crpc_id;
    };

    class ThroughputCalculator
    {
    public:
        ThroughputCalculator() = default;
        ThroughputCalculator(const ThroughputCalculator &) = delete;

        // ~ThroughputCalculator();

        SpinLock time_lock_;
        chrono::system_clock::time_point start_time;
        chrono::system_clock::time_point end_time;
        double latency;

        void add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time);

        // void calc_latency();

        double get_latency();
    };

    class DirectionThroughput
    {
    public:
        DirectionThroughput() = default;
        DirectionThroughput(const DirectionThroughput &) = delete;
        // ~ThroughPutManager();

        vector<shared_ptr<ThroughputCalculator>> dir_to_throughput_calculator;
        vector<RequestData> dir_to_throughput_data;

        SpinLock throughput_probe_lock_;
        int throughput_probe = -1;

        void add_request_start_time(uint64_t crpc_id, uint64_t direction);
        void add_request_end_time(uint64_t crpc_id);

        void calc_latency();

        double get_latency(uint64_t direction);

        void init_directions(int num_directions)
        {
            init_throughput_calculator(num_directions);
            init_throughput_store(num_directions);
        }

        void init_throughput_calculator(int num_directions)
        {
            if (dir_to_throughput_calculator.size() != 0)
            {
                return;
            }
            for (int i = 0; i < num_directions; i++)
            {
                dir_to_throughput_calculator.push_back(make_shared<ThroughputCalculator>());
            }
        }

        void init_throughput_store(int num_directions)
        {
            if (dir_to_throughput_data.size() != 0)
            {
                return;
            }
            for (int i = 0; i < num_directions; i++)
            {
                RequestData data;
                data.crpc_id = 0;
                dir_to_throughput_data.push_back(data);
            }
        }

        int get_throughput_probe()
        {
            throughput_probe_lock_.lock();
            int probe = throughput_probe;
            throughput_probe_lock_.unlock();
            return probe;
        }

        void decrement_throughput_probe()
        {
            throughput_probe_lock_.lock();
            Log_info("Setting throughput probe to %d", throughput_probe - 1);
            throughput_probe--;
            throughput_probe_lock_.unlock();
        }

        void reset_throughput_probe()
        {
            throughput_probe_lock_.lock();
            Log_info("Resetting throughput probe");
            throughput_probe = 1;
            throughput_probe_lock_.unlock();
        }

    };
} // namespace janus