#include "__dep__.h"

namespace janus
{
    struct ThroughputStore
    {
        chrono::system_clock::time_point start_time;
        chrono::system_clock::time_point end_time;
        uint64_t crpc_id;
    };

    enum ThroughputStatus
    {
        THROUGHPUT_STATUS_INVALID = -1,
        THROUGHPUT_STATUS_INIT = 0,
        THROUGHPUT_STATUS_START = 1,
        THROUGHPUT_STATUS_END = 2,
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
        double throughput;

        void add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time);

        void calc_throughput();

        double get_throughput();
    };

    class ThroughPutManager
    {
    public:
        ThroughPutManager() = default;
        ThroughPutManager(const ThroughPutManager &) = delete;
        // ~ThroughPutManager();

        vector<shared_ptr<ThroughputCalculator>> dir_to_throughput_calculator;
        vector<ThroughputStore> dir_to_throughput_store;

        SpinLock throughput_probe_lock_;
        int throughput_probe = THROUGHPUT_STATUS_INVALID;

        void add_request_start_time(uint64_t crpc_id, uint64_t direction);
        void add_request_end_time(uint64_t crpc_id);

        void calc_throughput();

        double get_throughput(uint64_t direction);

        void init_throughput_calculator(int num_directions)
        {
            for (int i = 0; i < num_directions; i++)
            {
                dir_to_throughput_calculator.push_back(make_shared<ThroughputCalculator>());
            }
        }

        void init_throughput_store(int num_directions)
        {
            for (int i = 0; i < num_directions; i++)
            {
                ThroughputStore store;
                store.crpc_id = 0;
                dir_to_throughput_store.push_back(store);
            }
        }

        bool get_throughput_probe()
        {
            throughput_probe_lock_.lock();
            int probe = throughput_probe;
            throughput_probe_lock_.unlock();
            return probe;
        }
        void set_throughput_probe(int probe)
        {
            Log_info("Setting throughput probe to %d", probe);
            throughput_probe_lock_.lock();
            throughput_probe = probe;
            throughput_probe_lock_.unlock();
        }
    };
} // namespace janus