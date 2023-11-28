#include "throughput.h"

namespace janus
{
    void ThroughPutManager::add_request_start_time(uint64_t crpc_id, uint64_t direction)
    {
        Log_info("add_request_start_time");
        if (dir_to_throughput_calculator.size() == 0)
        {
            Log_info("Initializing throughput calculator 10");
            init_throughput_calculator(2);
        }
        if (dir_to_throughput_store.size() == 0)
        {
            Log_info("Initializing throughput store 15");
            init_throughput_store(2);
        }
        if (get_throughput_probe() == THROUGHPUT_STATUS_END)
        {
            return;
        }
        if (dir_to_throughput_store[direction].crpc_id == 0)
        {
            dir_to_throughput_store[direction].crpc_id = crpc_id;
            dir_to_throughput_store[direction].start_time = chrono::system_clock::now();
            if (get_throughput_probe() == THROUGHPUT_STATUS_INIT)
            {
                set_throughput_probe(THROUGHPUT_STATUS_START);
            }
            else if (get_throughput_probe() == THROUGHPUT_STATUS_START)
            {
                set_throughput_probe(THROUGHPUT_STATUS_END);
            }
        }
        else
        {
            // This should not happen CHECK HOW TO HANDLE THIS
            // verify(false);
        }
    }

    void ThroughPutManager::add_request_end_time(uint64_t crpc_id)
    {
        Log_info("add_request_end_time");
        if (get_throughput_probe() == THROUGHPUT_STATUS_INIT)
        {
            return;
        }
        for (int i = 0; i < dir_to_throughput_store.size(); i++)
        {
            Log_debug("COUNTER %d", i);
            if (dir_to_throughput_store[i].crpc_id == crpc_id)
            {
                Log_info("add_request_END_TIME for crpc_id %lu", crpc_id);
                dir_to_throughput_store[i].end_time = chrono::system_clock::now();
                Log_info("HERE AT 36");
                dir_to_throughput_calculator[i]->add_request_times(dir_to_throughput_store[i].start_time, dir_to_throughput_store[i].end_time);
                Log_info("HERE AT 38");
                dir_to_throughput_store[i].crpc_id = 0;
                Log_info("HERE AT 40");
                return;
            }
        }
        // This should not happen
        // verify(false);
    }

    void ThroughputCalculator::add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time)
    {
        Log_debug("Acquiring lock");
        time_lock_.lock();
        Log_debug("Lock acquired");
        start_time = st_time;
        end_time = en_time;
        Log_debug("Releasing lock");
        time_lock_.unlock();
        Log_debug("Lock released");
        return;
    }

    // Call this only once
    void ThroughPutManager::calc_throughput()
    {
        Coroutine::CreateRun([this]()
                             {
            while (true)
            {
                set_throughput_probe(THROUGHPUT_STATUS_INIT);
                Log_info("Waiting for 1 second");
                auto ev = Reactor::CreateSpEvent<TimeoutEvent>(1000000);
                ev->Wait();
                Log_info("WAIT FINISH");
                if (dir_to_throughput_store.size() == 0)
                {
                    Log_info("Initializing throughput store 87");
                    init_throughput_store(2);
                }
                if (dir_to_throughput_calculator.size() == 0)
                {
                    Log_info("Initializing throughput calculator at 92");
                    init_throughput_calculator(2);
                }
                for (int i = 0; i < dir_to_throughput_calculator.size(); i++)
                {
                    Log_info("Calculating throughput for direction %d", i);
                    dir_to_throughput_calculator[i]->calc_throughput();
                }
                Log_info("Done");
            } });
    }

    void ThroughputCalculator::calc_throughput()
    {
        Log_debug("Acquiring lock");
        time_lock_.lock();
        Log_debug("Lock acquired");
        if (end_time == start_time)
        {
            Log_debug("Releasing lock");
            time_lock_.unlock();
            Log_debug("Lock released");
            return;
        }
        double time_taken = chrono::duration_cast<chrono::nanoseconds>(end_time - start_time).count();
        Log_debug("Releasing lock");
        time_lock_.unlock();
        Log_debug("Lock released");
        throughput = 1 / time_taken;
    }

    double ThroughPutManager::get_throughput(uint64_t direction)
    {
        if (dir_to_throughput_calculator.size() == 0)
        {
            return 0;
        }
        return dir_to_throughput_calculator[direction]->get_throughput();
    }

    double ThroughputCalculator::get_throughput()
    {
        Log_debug("Acquiring lock");
        time_lock_.lock();
        Log_debug("Lock acquired");
        start_time = chrono::system_clock::now();
        end_time = start_time;
        Log_debug("Releasing lock");
        time_lock_.unlock();
        Log_debug("Lock released");
        return throughput;
    }

} // namespace janus