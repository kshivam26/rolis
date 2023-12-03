#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../communicator.h"
#include "../throughput.h"
#include "chrono"

namespace janus
{

  class TxData;

  class MultiPaxosCommo : public Communicator
  {
  public:
    uint64_t crpc_id_counter = 0;
    bool direction = false;
    double dirProbability = 0.5;
    SpinLock dir_l_;

    shared_ptr<DirectionThroughput> dir_throughput_cal;
    uint64_t crpc_dir_0_counter = 0;
    uint64_t crpc_dir_1_counter = 0;

    chrono::system_clock::time_point last_checked_time;

    std::unordered_map<uint64_t, pair<function<void(ballot_t, int)>, shared_ptr<PaxosAcceptQuorumEvent>>>
        cRPCEvents{};
    SpinLock cRPCEvents_l_;
    MultiPaxosCommo() = delete;
    MultiPaxosCommo(PollMgr *);

    double getDirProbability();

    shared_ptr<PaxosPrepareQuorumEvent>
    BroadcastPrepare(parid_t par_id,
                     slotid_t slot_id,
                     ballot_t ballot);
    void BroadcastPrepare(parid_t par_id,
                          slotid_t slot_id,
                          ballot_t ballot,
                          const function<void(Future *fu)> &callback);
    shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastAccept(parid_t par_id,
                    slotid_t slot_id,
                    ballot_t ballot,
                    shared_ptr<Marshallable> cmd);
    void BroadcastAccept(parid_t par_id,
                         slotid_t slot_id,
                         ballot_t ballot,
                         shared_ptr<Marshallable> cmd,
                         const function<void(Future *)> &callback);
    void BroadcastDecide(const parid_t par_id,
                         const slotid_t slot_id,
                         const ballot_t ballot,
                         const shared_ptr<Marshallable> cmd);
    virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastBulkPrepare(parid_t par_id,
                         shared_ptr<Marshallable> cmd,
                         std::function<void(ballot_t, int)> cb) override;
    virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastHeartBeat(parid_t par_id,
                       shared_ptr<Marshallable> cmd,
                       const std::function<void(ballot_t, int)> &cb) override;

    shared_ptr<PaxosAcceptQuorumEvent>
    CrpcBroadcastHeartBeat(parid_t par_id,
                           shared_ptr<Marshallable> cmd,
                           const function<void(ballot_t, int)> &cb,
                           siteid_t id);
    virtual void
    CrpcHeartbeat(parid_t par_id,
                  uint64_t id,
                  MarshallDeputy cmd,
                  std::vector<uint16_t> &addrChain,
                  std::vector<BalValResult> &state);

    virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncNoOps(parid_t par_id,
                       shared_ptr<Marshallable> cmd,
                       const std::function<void(ballot_t, int)> &cb) override;

    virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncLog(parid_t par_id,
                     shared_ptr<Marshallable> cmd,
                     const std::function<void(shared_ptr<MarshallDeputy>, ballot_t, int)> &cb) override;

    virtual shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastSyncCommit(parid_t par_id,
                        shared_ptr<Marshallable> cmd,
                        const std::function<void(ballot_t, int)> &cb) override;

    shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastBulkAccept(parid_t par_id,
                        shared_ptr<Marshallable> cmd,
                        const std::function<void(ballot_t, int)> &cb);

    shared_ptr<PaxosAcceptQuorumEvent>
    CrpcBroadcastBulkAccept(parid_t par_id,
                            shared_ptr<Marshallable> cmd,
                            const std::function<void(ballot_t, int)> &cb,
                            siteid_t id);

    virtual void
    CrpcBulkAccept(parid_t par_id,
                   uint16_t recv_id,
                   uint64_t id,
                   MarshallDeputy cmd,
                   std::vector<uint16_t> &addrChain,
                   std::vector<BalValResult> &state);

    shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastBulkDecide(parid_t par_id,
                        const shared_ptr<Marshallable> cmd,
                        const std::function<void(ballot_t, int)> &cb);

    shared_ptr<PaxosAcceptQuorumEvent>
    CrpcBroadcastBulkDecide(parid_t par_id,
                            shared_ptr<Marshallable> cmd,
                            const std::function<void(ballot_t, int)> &cb,
                            siteid_t id);

    virtual void
    CrpcBulkDecide(parid_t par_id,
                   uint64_t id,
                   MarshallDeputy cmd,
                   std::vector<uint16_t> &addrChain,
                   std::vector<BalValResult> &state);

    shared_ptr<PaxosAcceptQuorumEvent>
    BroadcastPrepare2(parid_t par_id,
                      const shared_ptr<Marshallable> cmd,
                      const std::function<void(MarshallDeputy, ballot_t, int)> &cb);
  };

} // namespace janus
