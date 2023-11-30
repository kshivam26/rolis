#include "throughput.h"

namespace janus
{
    void DirectionThroughput::add_request_start_time(uint64_t crpc_id, uint64_t direction)
    {
        init_directions(2);
        if (get_throughput_probe() < 0)
        {
            return;
        }
        if (dir_to_throughput_data[direction].crpc_id == 0)
        {
            Log_info("Adding start time for crpc_id %lu and direction %lu", crpc_id, direction);
            dir_to_throughput_data[direction].crpc_id = crpc_id;
            dir_to_throughput_data[direction].start_time = chrono::system_clock::now();
        }
        else
        {
            // This should not happen CHECK HOW TO HANDLE THIS
            // verify(false);
        }
    }

    void DirectionThroughput::add_request_end_time(uint64_t crpc_id)
    {
        if (get_throughput_probe() < 0)
        {
            return;
        }
        // Log_info("Adding end time for crpc_id %lu", crpc_id);
        for (int i = 0; i < dir_to_throughput_data.size(); i++)
        {
            // Log_debug("COUNTER %d", i);
            if (dir_to_throughput_data[i].crpc_id == crpc_id)
            {
                Log_info("Adding end time for crpc_id %lu", crpc_id);
                dir_to_throughput_data[i].end_time = chrono::system_clock::now();
                dir_to_throughput_calculator[i]->add_request_times(dir_to_throughput_data[i].start_time, dir_to_throughput_data[i].end_time);
                dir_to_throughput_data[i].crpc_id = 0;
                decrement_throughput_probe();
                return;
            }
        }
        // This should not happen
        // verify(false);
    }

    void ThroughputCalculator::add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time)
    {
        start_time = st_time;
        end_time = en_time;
        double time_taken = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
        Log_info("Latency is %f", time_taken);
        latency = time_taken;
        return;
    }

    // Call this only once
    void DirectionThroughput::calc_latency()
    {
        Coroutine::CreateRun([this]()
                             {
            while (loop_var)
            {
                reset_throughput_probe();
                // Log_info("Waiting for 1 second");
                auto ev = Reactor::CreateSpEvent<TimeoutEvent>(1000000);
                ev->Wait();
                // Log_info("Waiting Finshed");
                // for (int i = 0; i < dir_to_throughput_calculator.size(); i++)
                // {
                //     Log_info("Calculating throughput for direction %d", i);
                //     dir_to_throughput_calculator[i]->calc_latency();
                // }
                // Log_info("Done");
            } });
    }

    double DirectionThroughput::get_latency(uint64_t direction)
    {
        if (dir_to_throughput_calculator.size() == 0)
        {
            return 0;
        }
        return dir_to_throughput_calculator[direction]->get_latency();
    }

    double ThroughputCalculator::get_latency()
    {
        return latency;
    }

} // namespace janus