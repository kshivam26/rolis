#include "throughput.h"
#include <random>
#include <cmath>

namespace janus
{
    void LatencyQueue::add(uint64_t id, std::chrono::system_clock::time_point start_time) {
        if (q.size() >= maxSize) {
            removeOldest();
        }
        q.push(RequestData(id, start_time));
        addToHeaps({HIGH_VALUE, id}); // Initially adding a high placeholder value
    }

    void LatencyQueue::updateEndTime(uint64_t id, std::chrono::system_clock::time_point end_time) {
        RequestData& request = q.back();
        if (request.crpc_id == id) {
            int old_latency = (request.end_time != std::chrono::system_clock::time_point::max())
                                ? chrono::duration_cast<chrono::microseconds>(request.end_time - request.start_time).count()
                                : HIGH_VALUE;

            if (old_latency != HIGH_VALUE) {
                removeLatencyFromHeaps({old_latency, id});
            }

            request.end_time = end_time;
            int latency = chrono::duration_cast<chrono::microseconds>(request.end_time - request.start_time).count();

            if (latency != HIGH_VALUE) {
                addToHeaps({latency, id});
            }

            // std::cout << "Updated end time for request " << id << ": " << request.end_time.time_since_epoch().count() << std::endl;
        }
        else{
            verify(0);
        }
        // while (!tempQueue.empty()) {
        //     RequestData& request = tempQueue.front();
        //     if (request.crpc_id == id) {
        //         int old_latency = (request.end_time != std::chrono::system_clock::time_point::max())
        //                             ? chrono::duration_cast<chrono::microseconds>(request.end_time - request.start_time).count()
        //                             : HIGH_VALUE;

        //         if (old_latency != HIGH_VALUE) {
        //             removeLatencyFromHeaps({old_latency, id});
        //         }

        //         request.end_time = end_time;
        //         int latency = chrono::duration_cast<chrono::microseconds>(request.end_time - request.start_time).count();

        //         if (latency != HIGH_VALUE) {
        //             addToHeaps({latency, id});
        //         }

        //         std::cout << "Updated end time for request " << id << ": " << request.end_time.time_since_epoch().count() << std::endl;
        //         break;
        //     }
        //     tempQueue.pop();
        // }
    }

    double LatencyQueue::findMedian() {
        if (q.size() == 0) {
            std::cerr << "Queue is empty!" << std::endl;
            return 0;
        }
        // display();
        if (q.size() % 2 == 0) {
            if (maxHeap.size() == minHeap.size()) {
                return (maxHeap.top().first + minHeap.top().first) / 2.0;
            } else {
                return maxHeap.top().first; // maxHeap always has one more element than minHeap
            }
        } else {
            return maxHeap.top().first;
        }
    }

    void LatencyQueue::removeOldest() {
        RequestData oldest = q.front();
        q.pop();

        int latency = (oldest.end_time != std::chrono::system_clock::time_point::max())
                          ? chrono::duration_cast<chrono::microseconds>(oldest.end_time - oldest.start_time).count()
                          : HIGH_VALUE;
        if (latency <= maxHeap.top().first) {
            removeFromHeap(maxHeap, {latency, oldest.crpc_id});
        } else {
            removeFromHeap(minHeap, {latency, oldest.crpc_id});
        }
    }

    void LatencyQueue::addToHeaps(std::pair<int, uint64_t> value) {
        if (value.first != HIGH_VALUE) {
            if (maxHeap.empty() || value.first <= maxHeap.top().first) {
                maxHeap.push(value);
            } else {
                minHeap.push(value);
            }

            rebalanceHeaps();
        }
    }

    void LatencyQueue::removeLatencyFromHeaps(std::pair<int, uint64_t> latencyId) {
        if (latencyId.first != HIGH_VALUE) {
            if (!maxHeap.empty() && latencyId.first <= maxHeap.top().first) {
                removeFromHeap(maxHeap, latencyId);
            } else if (!minHeap.empty() && latencyId.first >= minHeap.top().first) {
                removeFromHeap(minHeap, latencyId);
            }
        }
    }

    void LatencyQueue::removeFromHeap(std::priority_queue<std::pair<int, uint64_t>>& heap, std::pair<int, uint64_t> value) {
        std::priority_queue<std::pair<int, uint64_t>> tempHeap = heap;
        std::priority_queue<std::pair<int, uint64_t>> newHeap;

        while (!tempHeap.empty()) {
            if (tempHeap.top() != value) {
                newHeap.push(tempHeap.top());
            }
            tempHeap.pop();
        }

        heap = newHeap;

        rebalanceHeaps();
    }

    void LatencyQueue::removeFromHeap(std::priority_queue<std::pair<int, uint64_t>, std::vector<std::pair<int, uint64_t>>, std::greater<std::pair<int, uint64_t>>>& heap, std::pair<int, uint64_t> value) {
        std::priority_queue<std::pair<int, uint64_t>, std::vector<std::pair<int, uint64_t>>, std::greater<std::pair<int, uint64_t>>> tempHeap = heap;
        std::priority_queue<std::pair<int, uint64_t>, std::vector<std::pair<int, uint64_t>>, std::greater<std::pair<int, uint64_t>>> newHeap;

        while (!tempHeap.empty()) {
            if (tempHeap.top() != value) {
                newHeap.push(tempHeap.top());
            }
            tempHeap.pop();
        }

        heap = newHeap;

        rebalanceHeaps();
    }

    void LatencyQueue::rebalanceHeaps() {
        if (maxHeap.size() > minHeap.size() + 1) {
            minHeap.push(maxHeap.top());
            maxHeap.pop();
        } else if (minHeap.size() > maxHeap.size()) {
            maxHeap.push(minHeap.top());
            minHeap.pop();
        }
    }
    
    void LatencyQueue::display() {
        std::queue<RequestData> tempQueue = q; // Create a temporary copy of the queue

        while (!tempQueue.empty()) {
            const RequestData& request = tempQueue.front();
            std::cout << "Request ID: " << request.crpc_id << ", Start time: "
                    << std::chrono::duration_cast<std::chrono::microseconds>(request.start_time.time_since_epoch()).count()
                    << ", End time: ";
            if (request.end_time != std::chrono::system_clock::time_point::max()) {
                std::cout << std::chrono::duration_cast<std::chrono::microseconds>(request.end_time.time_since_epoch()).count();
            } else {
                std::cout << "Not available";
            }
            std::cout << std::endl;

            tempQueue.pop();
        }
    }

    RequestData LatencyQueue::getLastRequest(){
        if (q.size() > 0){
            auto value = q.back();
            return value;
        }
        else { 
            return RequestData(-1, std::chrono::system_clock::time_point::max(), std::chrono::system_clock::time_point::max());
        }
    }

    void DirectionThroughput::add_request_start_time(uint64_t crpc_id, uint64_t direction)
    {        
        if (get_throughput_probe() < 0)
        {
            return;
        }
        // case: when the previous probing request has received it's reply
        auto request = latency_queues[direction].getLastRequest();
        if(request.crpc_id == -1 || request.end_time != std::chrono::system_clock::time_point::max()){
            latency_queues[direction].add(crpc_id, chrono::system_clock::now());
            decrement_throughput_probe();
        }
        else{
            // Log_info("++++ UNLIKELY: The previous par_id: %ld, crpc_id: %ld and direction: %d", par_id_, request.crpc_id, direction);
            // witnessing that it sometimes takes 5-6 seconds for a request to come back through the slow node, in this case this code will trigger
            // This should not happen CHECK HOW TO HANDLE THIS
            if (direction == 0){
                // Log_info("++++ UNLIKELY: par_id: %ld, prob latency very high for direction 0; decrementing prob", par_id_);
                decrement_dir_prob();
            }
            else{
                // Log_info("++++ UNLIKELY: par_id: %ld, prob latency very high for direction 1; decrementing prob", par_id_);
                increment_dir_prob();
            }
            decrement_throughput_probe();
            return;
        }


        // if (latency_queues[direction].add(crpc) == 0)
        // {
        //     // Log_info("Adding start time for crpc_id %lu and direction %lu", crpc_id, direction);
        //     Log_info("#### Adding start time for par_id: %ld, crpc_id: %ld and direction: %d", par_id_, crpc_id, direction);
        //     dir_to_throughput_data[direction].crpc_id = crpc_id;
        //     dir_to_throughput_data[direction].start_time = chrono::system_clock::now();
        //     dir_to_throughput_data[direction].flag = true;
        //     decrement_throughput_probe();
        // }
        // else
        // {
        //     Log_info("#### UNLIKELY: The previous par_id: %ld, crpc_id: %ld and direction: %d", par_id_, dir_to_throughput_data[direction].crpc_id, direction);
        //     // witnessing that it sometimes takes 5-6 seconds for a request to come back through the slow node, in this case this code will trigger
        //     // This should not happen CHECK HOW TO HANDLE THIS
        //     if (direction == 0){
        //         Log_info("#### UNLIKELY: par_id: %ld, prob latency very high for direction 0; decrementing prob", par_id_);
        //         decrement_dir_prob();
        //     }
        //     else{
        //         Log_info("#### UNLIKELY: par_id: %ld, prob latency very high for direction 1; decrementing prob", par_id_);
        //         increment_dir_prob();
        //     }
        //     decrement_throughput_probe();
        //     return;
        //     // verify(0);
        // }
    }


    void DirectionThroughput::add_request_end_time(uint64_t crpc_id)
    {
        // if (get_throughput_probe() < 0)
        // {
        //     return;
        // }
        // Log_info("Adding end time for crpc_id %lu", crpc_id);
        for (int i = 0; i < latency_queues.size(); i++)
        {
            // Log_debug("COUNTER %d", i);
            if (latency_queues[i].getLastRequest().crpc_id == crpc_id)
            {
                // Log_info("++++ Adding end time for crpc_id: %ld and direction: %d", crpc_id, i);
                latency_queues[i].updateEndTime(crpc_id, chrono::system_clock::now());
                // dir_to_throughput_calculator[i]->add_request_times(dir_to_throughput_data[i].start_time, dir_to_throughput_data[i].end_time);
                // dir_to_throughput_data[i].crpc_id = 0;
                // decrement_throughput_probe();
                return;
            }
        }
        // This should not happen
        // verify(0);
    }

    // void ThroughputCalculator::add_request_times(chrono::system_clock::time_point st_time, chrono::system_clock::time_point en_time)
    // {
    //     start_time = st_time;
    //     end_time = en_time;
    //     double time_taken = chrono::duration_cast<chrono::microseconds>(end_time - start_time).count();
    //     Log_info("Latency is %f", time_taken);
    //     latency = time_taken;
    //     return;
    // }

    // Call this only once
    void DirectionThroughput::calc_latency(parid_t par_id)
    {
        if (started){
            Log_info("#### ATTENTION: Already started calc_latency for par_id: %ld", par_id_);
            return;
        }
        
        started = true;
        par_id_ = par_id;
        Coroutine::CreateRun([this](){
            // ensure all the threads do not start this coroutine at the same time
            // std::random_device rd;
            // std::mt19937 gen(rd());
            // std::uniform_int_distribution<> dis(1, 10);
            // int randomNum = dis(gen);
            // Log_info("#### The randomNum generated is: %d", randomNum);
            // auto start_ev = Reactor::CreateSpEvent<TimeoutEvent>(200000); // kshivam: change it to random number later
            // start_ev->Wait();
            Log_info("#### Starting calc_latency for par_id: %ld", par_id_);
            while (loop_var)
            {
                reset_throughput_probe();
                // Log_info("Waiting for 1 second");
                
                auto ev = Reactor::CreateSpEvent<TimeoutEvent>(timeout_period);
                ev->Wait();

                // in case where previous probing request has not been answer, latency is atleast current_time - start_time
                for(int i = 0; i < latency_queues.size(); i++){
                    auto last_request = latency_queues[i].getLastRequest();
                    if(last_request.end_time == std::chrono::system_clock::time_point::max()){
                        // Log_info("++++ Previous probing request for direction %d not returned; setting latency to current_time - start_time", i);
                        latency_queues[i].updateEndTime(last_request.crpc_id, chrono::system_clock::now());
                        // Log_info("++++ calc_latency; cp0; par_id: %ld, crpc_id: %d, start_time: %ld, end_time: %ld", par_id_, last_request.crpc_id, last_request.start_time, last_request.end_time);
                    }
                }
                                
                auto current_time = chrono::system_clock::now();
                auto shouldContinue = false;
                for(int i = 0; i < latency_queues.size(); i++){
                    auto last_request = latency_queues[i].getLastRequest();
                    if (chrono::duration_cast<chrono::microseconds>(current_time - last_request.start_time).count() > 1.25*timeout_period && chrono::duration_cast<chrono::microseconds>(current_time - last_request.end_time).count() > 1.2*timeout_period) {
                        // Log_info("++++ No probing has been sent in direction %d; par_id: %ld, for quite some time now, continuing", i, par_id_);
                        shouldContinue = true;
                    }
                }
                if (shouldContinue) continue;

                // Log_info("#### par_id: %ld, Direction 0; Start_time: %ld, End_time: %ld", par_id_, chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[0].start_time).count(), chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[0].end_time).count());
                // Log_info("#### par_id: %ld, Direction 1; Start_time: %ld, End_time: %ld", par_id_, chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[1].start_time).count(), chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[1].end_time).count());
                // if (chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[0].start_time).count() > 1.25*timeout_period && chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[0].end_time).count() > 2*timeout_period) {
                //     Log_info("No probing has been sent in direction 0; par_id: %ld, for quite some time now, continuing", par_id_);
                //     continue;
                // }
                // if (chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[1].start_time).count() > 1.25*timeout_period && chrono::duration_cast<chrono::microseconds>(current_time - dir_to_throughput_data[1].end_time).count() > 2*timeout_period) {
                //     Log_info("No probing has been sent in direction 1; par_id: %ld, for quite some time now, continuing", par_id_);
                //     continue;
                // }

                auto temp_dir_1_lat = latency_queues[0].findMedian();
                auto temp_dir_2_lat = latency_queues[1].findMedian();;
                
                double diff = abs(temp_dir_1_lat - temp_dir_2_lat);
                if (diff < 1000 || (temp_dir_1_lat == 0 && temp_dir_2_lat == 0))
                {
                    Log_info("par_id: %ld, diff is less than 1000 or both are 0", par_id_);
                    continue;
                }
                else if (temp_dir_1_lat > temp_dir_2_lat)
                {                       
                    // Log_info("par_id: %ld, temp_dir_1_lat: %f; temp_dir_2_lat: %f; temp_dir_1_lat > temp_dir_2_lat", par_id_, temp_dir_1_lat, temp_dir_2_lat);
                    // dir_prob = std::max(0.0, dir_prob - 0.1);
                    decrement_dir_prob(temp_dir_1_lat/temp_dir_2_lat);
                    // Log_info("++++ par_id: %ld, temp_dir_1_lat: %f; temp_dir_2_lat: %f; temp_dir_1_lat > temp_dir_2_lat", par_id_, temp_dir_1_lat, temp_dir_2_lat);
                    // Log_info("++++ par_id: %ld, current direction_prob is: %f", par_id_, dir_prob);
                }
                else
                {
                    // Log_info("par_id: %ld, temp_dir_1_lat: %f; temp_dir_2_lat: %f; temp_dir_1_lat < temp_dir_2_lat", par_id_, temp_dir_1_lat, temp_dir_2_lat);
                    // dir_prob = std::min(1.0, dir_prob + 0.1);
                    increment_dir_prob(temp_dir_2_lat/temp_dir_1_lat);
                    // Log_info("++++ par_id: %ld, temp_dir_1_lat: %f; temp_dir_2_lat: %f; temp_dir_1_lat < temp_dir_2_lat", par_id_, temp_dir_1_lat, temp_dir_2_lat);
                    // Log_info("++++ par_id: %ld, current direction_prob is: %f", par_id_, dir_prob);
                }
            } });
    }

    void DirectionThroughput::decrement_dir_prob(double factorToDecrement){
        double result = log10(factorToDecrement)*0.1;
        auto prevValue = dir_prob;
        if (dir_prob == 1.0 && result < 0.1){ // kshivam-tp: simplify this
            return;
        }
        dir_prob = std::max(0.0, dir_prob - result);
        if (prevValue == 0.0 && dir_prob == 0.0){
            incrementTimeout();
        }
        if (prevValue != dir_prob){
            resetTimeout();
        }
    }

    void DirectionThroughput::increment_dir_prob(double factorToIncrement){
        double result = log10(factorToIncrement)*0.1;
        auto prevValue = dir_prob;
        if (dir_prob == 0.0 && result < 0.1){
            return;
        }        
        dir_prob = std::min(1.0, dir_prob + result);
        if (prevValue == 1.0 && dir_prob == 1.0){
            incrementTimeout();
        }
        if (prevValue != dir_prob){
            resetTimeout();
        }
    }

    void DirectionThroughput::resetTimeout(){
        timeout_period = 1000000;
    }

    void DirectionThroughput::incrementTimeout(){
        timeout_period = std::min(timeout_period*2, 8000000);
    }
    
    double DirectionThroughput::get_dir_prob()
    {
        // Log_info("++++ Direction Probability is %f", dir_prob);
        return dir_prob;
    }

    // double DirectionThroughput::get_latency(uint64_t direction)
    // {
    //     if (dir_to_throughput_calculator.size() == 0)
    //     {
    //         return 0;
    //     }
    //     return dir_to_throughput_calculator[direction]->get_latency();
    // }

    // double ThroughputCalculator::get_latency()
    // {
    //     return latency;
    // }

} // namespace janus