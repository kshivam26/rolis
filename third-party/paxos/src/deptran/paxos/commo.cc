
#include "commo.h"
#include "../rcc/graph.h"
#include "../rcc/graph_marshaler.h"
#include "../command.h"
#include "../procedure.h"
#include "../command_marshaler.h"
#include "../rcc_rpc.h"

namespace janus
{
  thread_local bool hasPrinted = false;
  shared_ptr<DirectionThroughput> MultiPaxosCommo::dir_throughput_cal = make_shared<DirectionThroughput>();
  MultiPaxosCommo::MultiPaxosCommo(PollMgr *poll) : Communicator(poll)
  {
    //  verify(poll != nullptr);
    Log_info("#### Initializing MultiPaxosCommo");
    // dir_throughput_cal = make_shared<DirectionThroughput>();
  }

  // not used
  void MultiPaxosCommo::BroadcastPrepare(parid_t par_id,
                                         slotid_t slot_id,
                                         ballot_t ballot,
                                         const function<void(Future *)> &cb)
  {
    verify(0);
    // Log_debug("**** inside void BroadcastPrepare");
    auto proxies = rpc_par_proxies_[par_id];
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = cb;
      Future::safe_release(proxy->async_Prepare(slot_id, ballot, fuattr));
    }
  }

  // not used
  shared_ptr<PaxosPrepareQuorumEvent>
  MultiPaxosCommo::BroadcastPrepare(parid_t par_id,
                                    slotid_t slot_id,
                                    ballot_t ballot)
  {
    verify(0);
    // Log_debug("**** inside shared_ptr<PaxosPrepareQuorumEvent> MultiPaxosCommo::BroadcastPrepare");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    auto e = Reactor::CreateSpEvent<PaxosPrepareQuorumEvent>(n, n); // marker:ansh debug
    auto proxies = rpc_par_proxies_[par_id];
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, ballot](Future *fu)
      {
        ballot_t b = 0;
        fu->get_reply() >> b;
        e->FeedResponse(b == ballot);
        // TODO add max accepted value.
      };
      Future::safe_release(proxy->async_Prepare(slot_id, ballot, fuattr));
    }
    return e;
  }

  // not used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastAccept(parid_t par_id,
                                   slotid_t slot_id,
                                   ballot_t ballot,
                                   shared_ptr<Marshallable> cmd)
  {
    verify(0);
    // Log_debug("**** inside shared_ptr<PaxosAcceptQuorumEvent> MultiPaxosCommo::BroadcastAccept");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    //  auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, /2n/2+1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, n);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, ballot](Future *fu)
      {
        ballot_t b = 0;
        fu->get_reply() >> b;
        e->FeedResponse(b == ballot);
      };
      MarshallDeputy md(cmd);
      auto f = proxy->async_Accept(slot_id, ballot, md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // not used
  void MultiPaxosCommo::BroadcastAccept(parid_t par_id,
                                        slotid_t slot_id,
                                        ballot_t ballot,
                                        shared_ptr<Marshallable> cmd,
                                        const function<void(Future *)> &cb)
  {
    verify(0);
    // Log_debug("**** inside void BroadcastAccept");
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = cb;
      MarshallDeputy md(cmd);
      auto f = proxy->async_Accept(slot_id, ballot, md, fuattr);
      Future::safe_release(f);
    }
    //  verify(0);
  }

  // not used
  void MultiPaxosCommo::BroadcastDecide(const parid_t par_id,
                                        const slotid_t slot_id,
                                        const ballot_t ballot,
                                        const shared_ptr<Marshallable> cmd)
  {
    verify(0);
    // Log_debug("**** inside void BroadcastDecide");
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [](Future *fu) {};
      MarshallDeputy md(cmd);
      auto f = proxy->async_Decide(slot_id, ballot, md, fuattr);
      Future::safe_release(f);
    }
  }

  // not used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastBulkPrepare(parid_t par_id,
                                        shared_ptr<Marshallable> cmd,
                                        function<void(ballot_t, int)> cb)
  {
    // Log_debug("**** BroadcastBulkPrepare: i am here"); // not used
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k); // marker:debug
    // Log_debug("BroadcastBulkPrepare: i am here partition size %d", n);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        // Log_debug("Received response %d %d", ballot, valid);
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_BulkPrepare(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastPrepare2(parid_t par_id,
                                     shared_ptr<Marshallable> cmd,
                                     const std::function<void(MarshallDeputy, ballot_t, int)> &cb)
  {
    // Log_info("#### BroadcastBulkPrepare2: i am here");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k); // marker:debug
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    // Log_info("paxos commo bulkaccept: length proxies %d", proxies.size());
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        MarshallDeputy response_val;
        fu->get_reply() >> ballot >> valid >> response_val;
        // Log_info("++++ BroadcastPrepare2: received response: %d %d", ballot, valid);
        // if (valid == 1)
        //   Log_info("++++ valid received is 1; BroadcastPrepare2: received response: %d %d with gettid: %d", ballot, valid, gettid());
        // if (!response_val.sp_data_)
        //   Log_info("++++ response_val.sp_data_ is null");
        cb(response_val, ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_BulkPrepare2(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastHeartBeat(parid_t par_id,
                                      shared_ptr<Marshallable> cmd,
                                      const function<void(ballot_t, int)> &cb)
  {
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_Heartbeat(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::CrpcBroadcastHeartBeat(parid_t par_id,
                                          shared_ptr<Marshallable> cmd,
                                          const function<void(ballot_t, int)> &cb,
                                          siteid_t leader_site_id)
  {
    Log_debug("inside CrpcBroadcastHeartbeat");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];

    std::vector<uint16_t> sitesInfo_;

    for (auto &p : proxies)
    {
      auto id = p.first;
      Log_debug("id is: %d and leader_site_id is: %d", id, leader_site_id);
      if (id != leader_site_id)
      {                           
        sitesInfo_.push_back(id); 
      }                           
    }

    sitesInfo_.push_back(leader_site_id);

    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      // FutureAttr fuattr;
      // fuattr.callback = [e, cb] (Future* fu) {
      //   i32 valid;
      //   i32 ballot;
      //   fu->get_reply() >> ballot >> valid;
      //   cb(ballot, valid);
      //   e->FeedResponse(valid);
      // };
      if (p.first != sitesInfo_[0])
      {
        continue;
      }
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      std::vector<BalValResult> state;
      uint64_t crpc_id = reinterpret_cast<uint64_t>(&e);
      // // Log_info("*** crpc_id is: %d", crpc_id); // verify it's never the same
      cRPCEvents_l_.lock();
      verify(cRPCEvents.find(crpc_id) == cRPCEvents.end());
      cRPCEvents[crpc_id] = std::make_pair(cb, e);
      cRPCEvents_l_.unlock();

      auto f = proxy->async_CrpcHeartbeat(crpc_id, md, sitesInfo_, state);
      Future::safe_release(f);

      break;
    }
    return e;
  }

  void MultiPaxosCommo::CrpcHeartbeat(parid_t par_id,
                                      uint64_t id,
                                      MarshallDeputy cmd,
                                      std::vector<uint16_t> &addrChain,
                                      std::vector<BalValResult> &state)
  {
    // Log_debug("**** inside MultiPaxosCommo::CrpcHeartbeat");
    auto proxies = rpc_par_proxies_[par_id];
    for (auto &p : proxies)
    {
      if (p.first != addrChain[0])
      {
        continue;
      }
      // Log_debug("**** inside MultiPaxosCommo::CrpcHeartbeat; p.first:%d", p.first);
      auto proxy = (MultiPaxosProxy *)p.second;
      auto f = proxy->async_CrpcHeartbeat(id, cmd, addrChain, state);
      Future::safe_release(f);
      // Log_debug("**** returning MultiPaxosCommo::CrpcHeartbeat");
      break;
    }
  }

  // not used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastSyncLog(parid_t par_id,
                                    shared_ptr<Marshallable> cmd,
                                    const std::function<void(shared_ptr<MarshallDeputy>, ballot_t, int)> &cb)
  {
    Log_debug("**** inside BroadcastSyncLog");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        MarshallDeputy response_val;
        fu->get_reply() >> ballot >> valid >> response_val;
        auto sp_md = make_shared<MarshallDeputy>(response_val);
        cb(sp_md, ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_SyncLog(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // not used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastSyncNoOps(parid_t par_id,
                                      shared_ptr<Marshallable> cmd,
                                      const std::function<void(ballot_t, int)> &cb)
  {
    // Log_debug("**** inside BroadcastSyncNoOps");
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_SyncNoOps(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // not used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastSyncCommit(parid_t par_id,
                                       shared_ptr<Marshallable> cmd,
                                       const std::function<void(ballot_t, int)> &cb)
  {
    Log_debug("**** inside BroadcastSyncCommit");
    // Log_debug("**** inside BroadcastSyncCommit, with size of cmd is: %d", sizeof(cmd));
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      auto f = proxy->async_SyncCommit(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  // used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastBulkAccept(parid_t par_id,
                                       shared_ptr<Marshallable> cmd,
                                       const function<void(ballot_t, int)> &cb)
  {
    // Log_debug("**** inside BroadcastBulkAccept");
    static bool hasPrinted = false; // Static variable to track if it has printed

    if (!hasPrinted)
    {
      Log_info("in no cRPC;");
      // Log_debug("in no cRPC; tid of leader is %d", gettid());
      hasPrinted = true; // Update the static variable
    }
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k); // marker:debug
    auto proxies = rpc_par_proxies_[par_id];
    vector<Future *> fus;
    // Log_debug("Sending bulk accept for some slot");
    // Log_debug("paxos commo bulkaccept: length proxies %d", proxies.size());
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      int st = p.first;
      fuattr.callback = [e, cb, st](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        Log_debug("Accept response received from %d site", st);
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      // Log_debug("Sending bulk accept for some slot");
      auto f = proxy->async_BulkAccept(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::CrpcBroadcastBulkAccept(parid_t par_id,
                                           shared_ptr<Marshallable> cmd,
                                           const function<void(ballot_t, int)> &cb,
                                           siteid_t leader_site_id)
  {
    // Log_info("**** inside CrpcBroadcastBulkAccept, with par_id: %d", par_id);
    // Log_info("**** inside CrpcBroadcastBulkAccept, with size of cmd is: %d", sizeof(cmd));
    // static bool hasPrinted = false;  // Static variable to track if it has printed

    // bool dynamic = true;
    // bool alternate = false;
    // bool slow = false;
    // bool fast = false;

    if (!hasPrinted)
    {
      Log_info("in cRPC;");
      if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::DYNAMIC)){
        Log_info("#### dynamic routing");
        if (par_id == 0) 
          dir_throughput_cal->calc_latency(par_id); // dynamic: uncomment
      } 
      else if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::ALTERNATE)) Log_info("#### alternate routing");
      else if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::SLOW)) Log_info("#### slow routing");
      else if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::FAST)) Log_info("#### fast routing");
      // Log_info("in cRPC; par_id:%d, cpu: %d", par_id, sched_getcpu());
      hasPrinted = true; // Update the static variable      
    }
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);
    auto proxies = rpc_par_proxies_[par_id];

    std::vector<uint16_t> sitesInfo_;

    sitesInfo_.push_back(leader_site_id);

    // dynamic: uncomment
    if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::DYNAMIC)){
      auto current_throughput_probe_status = dir_throughput_cal->get_throughput_probe();
      // Log_info("++++ par_id: %ld; current_throughput_probe_status: %d", par_id, current_throughput_probe_status);
      if (par_id == 0 && current_throughput_probe_status >= 0) {
        // Log_info("++++ par_id: %ld; inside cp1", par_id);
        if (current_throughput_probe_status == 0) {
          direction = true;
          for (auto it = proxies.rbegin(); it != proxies.rend(); ++it) {
            auto id = it->first; // Access the element through the reverse iterator
            if (id != leader_site_id) {
              sitesInfo_.push_back(id);
            }
          }
        }
        else if (current_throughput_probe_status == 1) {
          for (auto &p : proxies) {
            direction = false;
            auto id = p.first;
            // Log_info("**** id is: %d and leader_site_id is: %d", id, leader_site_id);
            if (id != leader_site_id) {                           
              sitesInfo_.push_back(id); 
            }                           
          }
        }
      }
      else {
        // Log_info("++++ par_id: %ld; inside cp2", par_id);
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator(seed);
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        double randomValue = distribution(generator);
        auto tempDirProbability = dir_throughput_cal->get_dir_prob();
        // Log_info("++++ par_id: %d; current dirProbability: %f", par_id, tempDirProbability);
        // Log_info("++++ par_id: %d; current randomValue: %f", par_id, randomValue);
        if (randomValue < tempDirProbability) {
          // Log_info("In first direction");
          // This is used for marking the direction in which the request is sent
          direction = true;
          for (auto it = proxies.rbegin(); it != proxies.rend(); ++it) {
            auto id = it->first; // Access the element through the reverse iterator
            if (id != leader_site_id) {
              sitesInfo_.push_back(id);
            }
          }
        }
        else {
          direction = false;
          // Log_info("In second direction");
          for (auto &p : proxies) {
            auto id = p.first;
            // Log_info("**** id is: %d and leader_site_id is: %d", id, leader_site_id);
            if (id != leader_site_id) {                           
              sitesInfo_.push_back(id); 
            }                           
          }
        }
      }
    }

    // dynamic: comment
    if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::ALTERNATE)){
      if (direction) {
        // Log_info("In first direction");
        // This is used for marking the direction in which the request is sent
        direction = false;
        for (auto it = proxies.rbegin(); it != proxies.rend(); ++it)
        {
          auto id = it->first; // Access the element through the reverse iterator
          if (id != leader_site_id)
          {
            sitesInfo_.push_back(id);
          }
        }
      }
      else {
        direction = true;
        // Log_info("In second direction");
        for (auto &p : proxies) {
          auto id = p.first;
          // Log_info("**** id is: %d and leader_site_id is: %d", id, leader_site_id);
          if (id != leader_site_id) {                           
            sitesInfo_.push_back(id); 
          }                           
        }
      }
    }
    // Normal CRPC code without directions // dynamic: comment
    if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::FAST)){
      for (auto &p : proxies) {
        auto id = p.first;
        // Log_info("**** id is: %d and leader_site_id is: %d", id, leader_site_id);
        if (id != leader_site_id) {                           
          sitesInfo_.push_back(id); 
        }                           
      }
    }
    if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::SLOW)){
      for (auto it = proxies.rbegin(); it != proxies.rend(); ++it) { // dynamic: comment 
        auto id = it->first; // Access the element through the reverse iterator
        if (id != leader_site_id) {
          sitesInfo_.push_back(id);
        }
      }
    }

    sitesInfo_.push_back(leader_site_id);

    verify(sitesInfo_[0] == leader_site_id); // kshivam: delete later

    for (auto &p : proxies) { // kshivam: optimize later to not loop over proxies; save leader proxy in previous step
      auto proxy = (MultiPaxosProxy *)p.second;
      if (p.first == sitesInfo_[0]) {
        verify(cmd != nullptr);
        MarshallDeputy md(cmd);
        // Log_info("**** inside CrpcBroadcastBulkAccept, with size of md is: %d", sizeof(md));
        std::vector<BalValResult> state;
        // int sizeA = sizeof(&e);
        // int sizeB = sizeof(uint64_t);
        // verify(sizeA == sizeB);
        uint64_t crpc_id = ++crpc_id_counter;

        // dynamic: uncomment
        if (Config::GetConfig()->getRoutingOption() == static_cast<int>(RoutingOptions::DYNAMIC)){
          auto current_throughput_probe_status = dir_throughput_cal->get_throughput_probe();
          if (par_id == 0 && current_throughput_probe_status >= 0) {
            // Mark this crpc_id in the store based on direction
            if (direction) {      
              // Log_info("++++ calling start time for par_id: %ld, crpc_id: %ld and direction: 0", par_id, crpc_id);      
              dir_throughput_cal->add_request_start_time(crpc_id, 0);
            }
            else {
              // Log_info("++++ calling start time for par_id: %ld, crpc_id: %ld and direction: 1", par_id, crpc_id);
              dir_throughput_cal->add_request_start_time(crpc_id, 1);
            }
          }
          if (direction) {
            crpc_dir_0_counter++;
          }
          else {
            crpc_dir_1_counter++;
          }

          // Log_info("++++ par_id: %ld, CRPC DIR 0 COUNTER: %d", par_id, crpc_dir_0_counter);
          // Log_info("++++ par_id: %ld, CRPC DIR 1 COUNTER: %d", par_id, crpc_dir_1_counter);
        }
        cRPCEvents_l_.lock();
        verify(cRPCEvents.find(crpc_id) == cRPCEvents.end());
        cRPCEvents[crpc_id] = std::make_pair(cb, e);
        cRPCEvents_l_.unlock();
        // Log_info("++++ sending request par_id: %ld, crpc_id: %ld and direction: %d", par_id, crpc_id, !direction);
        auto f = proxy->async_CrpcBulkAccept(crpc_id, md, sitesInfo_, state);
        Future::safe_release(f);
        break;
      }
    }
    return e;
  }

  void MultiPaxosCommo::CrpcBulkAccept(parid_t par_id,
                                       uint16_t recv_id,
                                       uint64_t id,
                                       MarshallDeputy cmd,
                                       std::vector<uint16_t> &addrChain,
                                       std::vector<BalValResult> &state)
  {
    // Log_info("#### inside MultiPaxosCommo::CrpcBulkAccept cp0 with par_id: %d", par_id);
    // Log_info("#### MultiPaxosCommo::CrpcBulkAccept; cp 0 with par_id:%d, crpc_id: %ld", par_id, id);
    auto proxies = rpc_par_proxies_[par_id];
    for (auto &p : proxies)
    {
      if (p.first == recv_id)
      {
        auto proxy = (MultiPaxosProxy *)p.second;
        // Log_info("#### sending crpcBulkAccept request; par_id: %d", par_id);
        auto f = proxy->async_CrpcBulkAccept(id, cmd, addrChain, state);
        if (f == nullptr)
        {
          // Log_info("############################ UNLIKELY, future is nullptr");
        }
        Future::safe_release(f);
        break;
      }
    }
    // Log_info("#### MultiPaxosCommo::CrpcBulkAccept; cp 2 with par_id:%d, crpc_id: %ld", par_id, id);
  }

  // used
  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::BroadcastBulkDecide(parid_t par_id,
                                       shared_ptr<Marshallable> cmd,
                                       const function<void(ballot_t, int)> &cb)
  {
    // Log_debug("**** inside BroadcastBulkDecide");
    // Log_debug("**** inside BroadcastBulkDecide, with size of cmd is: %d", sizeof(cmd));
    auto proxies = rpc_par_proxies_[par_id];
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k); // marker:debug
    vector<Future *> fus;
    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      FutureAttr fuattr;
      fuattr.callback = [e, cb](Future *fu)
      {
        i32 valid;
        i32 ballot;
        fu->get_reply() >> ballot >> valid;
        cb(ballot, valid);
        e->FeedResponse(valid);
      };
      MarshallDeputy md(cmd);
      auto f = proxy->async_BulkDecide(md, fuattr);
      Future::safe_release(f);
    }
    return e;
  }

  shared_ptr<PaxosAcceptQuorumEvent>
  MultiPaxosCommo::CrpcBroadcastBulkDecide(parid_t par_id,
                                           shared_ptr<Marshallable> cmd,
                                           const function<void(ballot_t, int)> &cb,
                                           siteid_t leader_site_id)
  {
    // Log_debug("**** inside CrpcBroadcastBulkDecide, with par_id: %d", par_id);
    // Log_debug("**** inside CrpcBroadcastBulkDecide, with size of cmd is: %d", sizeof(cmd));
    auto proxies = rpc_par_proxies_[par_id];
    int n = Config::GetConfig()->GetPartitionSize(par_id);
    int k = (n % 2 == 0) ? n / 2 : (n / 2 + 1);
    auto e = Reactor::CreateSpEvent<PaxosAcceptQuorumEvent>(n, k);

    std::vector<uint16_t> sitesInfo_;

    for (auto &p : proxies)
    {
      auto id = p.first;
      // Log_debug("**** CrpcBroadcastBulkDecide; id is: %d and leader_site_id is: %d", id, leader_site_id);
      if (id != leader_site_id)
      {                           
        sitesInfo_.push_back(id); 
      }                           
    }

    sitesInfo_.push_back(leader_site_id);

    for (auto &p : proxies)
    {
      auto proxy = (MultiPaxosProxy *)p.second;
      if (p.first != sitesInfo_[0])
      {
        continue;
      }
      verify(cmd != nullptr);
      MarshallDeputy md(cmd);
      std::vector<BalValResult> state;
      uint64_t crpc_id = reinterpret_cast<uint64_t>(&e);
      // // Log_debug("*** crpc_id is: %d", crpc_id); // verify it's never the same
      verify(cRPCEvents.find(crpc_id) == cRPCEvents.end());

      auto f = proxy->async_CrpcBulkDecide(crpc_id, md, sitesInfo_, state);
      Future::safe_release(f);
      cRPCEvents[crpc_id] = std::make_pair(cb, e);
      break;
    }
    return e;
  }

  void MultiPaxosCommo::CrpcBulkDecide(parid_t par_id,
                                       uint64_t id,
                                       MarshallDeputy cmd,
                                       std::vector<uint16_t> &addrChain,
                                       std::vector<BalValResult> &state)
  {
    // Log_debug("**** inside MultiPaxosCommo::CrpcBulkDecide");
    auto proxies = rpc_par_proxies_[par_id];
    for (auto &p : proxies)
    {
      if (p.first != addrChain[0])
      {
        continue;
      }
      // Log_debug("**** inside MultiPaxosCommo::CrpcBulkDecide; p.first:%d", p.first);
      auto proxy = (MultiPaxosProxy *)p.second;
      auto f = proxy->async_CrpcBulkDecide(id, cmd, addrChain, state);
      Future::safe_release(f);
      // Log_debug("**** returning MultiPaxosCommo::CrpcBulkDecide");
      break;
    }
  }

} // namespace janus
