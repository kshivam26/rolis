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
            // Log_info("Adding start time for crpc_id %lu and direction %lu", crpc_id, direction);
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
                // Log_info("Adding end time for crpc_id %lu", crpc_id);
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
                auto ev = Reactor::CreateSpEvent<TimeoutEvent>(1500000);
                ev->Wait();
                double temp_dir_1_lat = dir_to_throughput_calculator[0]->get_latency();
                double temp_dir_2_lat = dir_to_throughput_calculator[1]->get_latency();
                double diff = abs(temp_dir_1_lat - temp_dir_2_lat);
                if (diff < 1000 || (temp_dir_1_lat == 0 && temp_dir_2_lat == 0))
                {
                    Log_info("diff is less than 1000 or both are 0");
                    // if (dir_prob == 0.5)
                    // {
                    //     continue;
                    // }
                    // else if (dir_prob < 0.5)
                    // {
                    //     dir_prob = std::min(1.0, dir_prob + 0.1);
                    // }
                    // else
                    // {
                    //     dir_prob = std::max(0.0, dir_prob - 0.1);
                    // }
                    continue;
                }
                else if (temp_dir_1_lat > temp_dir_2_lat)
                {
                    Log_info("temp_dir_1_lat > temp_dir_2_lat");
                    dir_prob = std::max(0.0, dir_prob - 0.1);
                }
                else
                {
                    Log_info("temp_dir_1_lat < temp_dir_2_lat");
                    dir_prob = std::min(1.0, dir_prob + 0.1);
                }
            } });
    }

    double DirectionThroughput::get_dir_prob()
    {
        Log_info("Direction Probability is %f", dir_prob);
        return dir_prob;
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