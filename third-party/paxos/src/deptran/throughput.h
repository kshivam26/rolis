#include "__dep__.h"
#include <iostream>
#include <queue>
#include <functional>
// #include <chrono>

namespace janus
{
    const int HIGH_VALUE = 1e9; // A high placeholder value for end_time

    struct RequestData {
        uint64_t crpc_id;
        std::chrono::system_clock::time_point start_time;
        std::chrono::system_clock::time_point end_time;

        RequestData(uint64_t identifier, std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end = std::chrono::system_clock::time_point::max()) : crpc_id(identifier), start_time(start), end_time(end) {}
    };

    class LatencyQueue {
    public:
        LatencyQueue(size_t n): maxSize(n){}

        void add(uint64_t id, std::chrono::system_clock::time_point start_time);
        void updateEndTime(uint64_t id, std::chrono::system_clock::time_point end_time);
        double findMedian();
        void display();
        RequestData getLastRequest();

    private:
        std::queue<RequestData> q;
        size_t maxSize;
        std::priority_queue<std::pair<int, uint64_t>> maxHeap; // {latency, id}
        std::priority_queue<std::pair<int, uint64_t>, std::vector<std::pair<int, uint64_t>>, std::greater<std::pair<int, uint64_t>>> minHeap; // {latency, id}
        uint64_t requestCounter;

        void removeOldest();
        void addToHeaps(std::pair<int, uint64_t> value);
        void removeLatencyFromHeaps(std::pair<int, uint64_t> latencyId);
        void removeFromHeap(std::priority_queue<std::pair<int, uint64_t>>& heap, std::pair<int, uint64_t> value);
        void removeFromHeap(std::priority_queue<std::pair<int, uint64_t>, std::vector<std::pair<int, uint64_t>>, std::greater<std::pair<int, uint64_t>>>& heap, std::pair<int, uint64_t> value);
        void rebalanceHeaps();
    };

    class ThroughputCalculator
    {
    public:
        ThroughputCalculator() = default;
        // ThroughputCalculator(const ThroughputCalculator &) = delete;

        // ~ThroughputCalculator();

        SpinLock time_lock_;
        chrono::system_clock::time_point start_time;
        chrono::system_clock::time_point end_time;
        double latency = 0.0;

        void add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time);

        // void calc_latency();

        // double get_latency();
    };

    class DirectionThroughput
    {
    public:
        DirectionThroughput() {init_directions(2);}
        DirectionThroughput(const DirectionThroughput &) {init_directions(2);}
        // ~ThroughPutManager();
        
        vector<LatencyQueue> latency_queues;
        vector<ThroughputCalculator*> dir_to_throughput_calculator;
        vector<RequestData> dir_to_throughput_data;
        int timeout_period = 1000000;
        SpinLock throughput_probe_lock_;
        int throughput_probe = -1;
        bool loop_var = true;
        double dir_prob = 0.5;
        parid_t par_id_= -1;
        bool started = false;
        void add_request_start_time(uint64_t crpc_id, uint64_t direction);
        void add_request_end_time(uint64_t crpc_id);
        void decrement_dir_prob(double factorToDecrement=0.5);
        void increment_dir_prob(double factorToIncrement=0.5);
        void calc_latency(parid_t par_id);

        void incrementTimeout();
        void resetTimeout();
        double get_dir_prob();

        // double get_latency(uint64_t direction);

        void init_directions(int num_directions)
        {
            // init_throughput_calculator(num_directions);
            // init_throughput_store(num_directions);
            init_latency_queues(num_directions);
        }

        void init_latency_queues(int num_directions){
            for (int i = 0; i < num_directions; i++)
            {
                LatencyQueue q(7);
                latency_queues.push_back(q);                
            }
        }

        // void init_throughput_calculator(int num_directions)
        // {
        //     if (dir_to_throughput_calculator.size() != 0)
        //     {
        //         return;
        //     }
        //     for (int i = 0; i < num_directions; i++)
        //     {
        //         auto newObj = new ThroughputCalculator();
        //         dir_to_throughput_calculator.push_back(newObj);
        //     }
        // }

        // void init_throughput_store(int num_directions)
        // {
        //     if (dir_to_throughput_data.size() != 0)
        //     {
        //         return;
        //     }
        //     for (int i = 0; i < num_directions; i++)
        //     {
        //         RequestData data;
        //         data.crpc_id = 0;
        //         dir_to_throughput_data.push_back(data);
        //     }
        // }

        int get_throughput_probe()
        {
            throughput_probe_lock_.lock();
            int probe = throughput_probe;
            // Log_info("++++ inside get_throughput_probe, with par_id: %ld, probe: %d", par_id_, probe);
            throughput_probe_lock_.unlock();
            return probe;
        }

        void decrement_throughput_probe()
        {
            throughput_probe_lock_.lock();
            // Log_info("Setting throughput probe to %d", throughput_probe - 1);
            throughput_probe--;
            throughput_probe_lock_.unlock();
        }

        void reset_throughput_probe()
        {
            throughput_probe_lock_.lock();
            // Log_info("Resetting throughput probe");
            throughput_probe = 1;
            throughput_probe_lock_.unlock();
        }

    };
} // namespace janus