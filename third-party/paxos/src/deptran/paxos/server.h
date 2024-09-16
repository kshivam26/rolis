#pragma once

#include "../__dep__.h"
#include "../constants.h"
#include "../scheduler.h"
#include "../paxos_worker.h"
#include <iostream>

namespace janus {

class Command;
class CmdData;

struct PaxosData {
  ballot_t max_ballot_seen_ = 0;
  ballot_t max_ballot_accepted_ = 0;
  bool is_no_op = false;
  shared_ptr<Marshallable> accepted_cmd_{nullptr};
  shared_ptr<Marshallable> committed_cmd_{nullptr};
};

struct BulkPrepare{
  ballot_t seen_ballot;
  int leader_id;
};

class PaxosServer : public TxLogServer {
 public:
  slotid_t min_active_slot_ = 0; // anything before (lt) this slot is freed
  slotid_t max_executed_slot_ = 0;
  slotid_t max_committed_slot_ = 0;
  slotid_t cur_min_prepared_slot_ = 0;
  slotid_t max_accepted_slot_ = 0;
  slotid_t max_possible_slot_ = INT_MAX;
  slotid_t cur_open_slot_ = 1;
  slotid_t max_touched_slot = 0;
  int leader_id;
  map<pair<slotid_t, slotid_t>, BulkPrepare> bulk_prepares{};  // saves all the prepare ranges.
  map<slotid_t, shared_ptr<PaxosData>> logs_{};

  map<slotid_t, std::shared_ptr<rrr::Coroutine>> commit_coro{};
  map<slotid_t, shared_ptr<IntEvent>> commit_ev{};
  vector<std::shared_ptr<rrr::Coroutine>> ready_commit_coro;
  SpinLock commit_ev_l_;
  set<slotid_t> accepted_slots;
  ballot_t cur_epoch;

  int n_prepare_ = 0;
  int n_accept_ = 0;
  int n_commit_ = 0;

  bool first_time = true;

  ~PaxosServer() {
    Log_info("site par %d, loc %d: prepare %d, accept %d, commit %d", partition_id_, loc_id_, n_prepare_, n_accept_, n_commit_);
  }

  shared_ptr<PaxosData> GetInstance(slotid_t id) {
    verify(id >= min_active_slot_);
    // Log_info("#### the current par_id:%d, slot_id: %ld and value is going to be accessed", partition_id_, id);
    auto& sp_instance = logs_[id];
    if(!sp_instance){
      // Log_info("#### inside GetInstance; sp_instance is null for par_id:%d, slot_id: %ld", partition_id_, id);
      sp_instance = std::make_shared<PaxosData>();
    }
    // else{
    //   // Log_info("**** the value of max_ballot_accepted: %d", sp_instance->max_ballot_accepted_);
    // }
    // Log_info("#### returning from GetInstance for par_id:%d, slot_id: %ld", partition_id_, id);
    return sp_instance;
  }

  void OnPrepare(slotid_t slot_id,
                 ballot_t ballot,
                 ballot_t *max_ballot,
                 const function<void()> &cb);

  void OnAccept(const slotid_t slot_id,
                const ballot_t ballot,
                shared_ptr<Marshallable> &cmd,
                ballot_t *max_ballot,
                const function<void()> &cb);

  void OnCommit(const slotid_t slot_id,
                const ballot_t ballot,
                shared_ptr<Marshallable> &cmd);

  void OnBulkPrepare(shared_ptr<Marshallable> &cmd,
                    i32 *ballot,
                    i32* valid,
                    const function<void()> &cb);

  void OnHeartbeat(shared_ptr<Marshallable> &cmd,
                    i32 *ballot,
                    i32* valid,
                    const function<void()> &cb);

  void OnCrpcHeartbeat(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state);

  void OnBulkAccept(shared_ptr<Marshallable> &cmd,
                    i32* ballot,
                    i32 *valid,
                    const function<void()> &cb);

  void OnCrpcBulkAccept(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state);

  void OnBulkCommit(shared_ptr<Marshallable> &cmd,
                    i32* ballot,
                    i32 *valid,
                    const function<void()> &cb);

  void OnCrpcBulkCommit(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state);

  void OnBulkPrepare2(shared_ptr<Marshallable> &cmd,
                      i32* ballot,
                      i32 *valid,
                      shared_ptr<BulkPaxosCmd> ret_cmd,
                      const function<void()> &cb);

  void OnSyncLog(shared_ptr<Marshallable> &cmd,
                      i32* ballot,
                      i32 *valid,
                      shared_ptr<SyncLogResponse> ret_cmd,
                      const function<void()> &cb);

  void OnSyncCommit(shared_ptr<Marshallable> &cmd,
                      i32* ballot,
                      i32 *valid,
                      const function<void()> &cb);


  void OnSyncNoOps(shared_ptr<Marshallable> &cmd,
                  i32* ballot,
                  i32 *valid,
                  const function<void()> &cb);

  void RunPendingCommitCoroutine(){
    for(auto coro: ready_commit_coro){
      // Log_info("#### PaxosServer::RunPendingCommitCoroutine; running coroutine from the ready_commit_coro vector; coro_id: %p", coro.get());
      coro->Continue();
    }

    // Log_info("#### PaxosServer::RunPendingCommitCoroutine; clearing ready_commit_coro vector");
    ready_commit_coro.clear();
  }

  bool NotEndCmd(shared_ptr<Marshallable> &cmd);

  int get_open_slot(){
    return cur_open_slot_++;
  }

  void FreeSlots(){
    // TODO should support snapshot for freeing memory.
    // for now just free anything 1000 slots before.
    int i = min_active_slot_;
    while (i + 100 < max_executed_slot_) {
      Log_info("Erasing entry number %d", i);
      logs_.erase(i);
      i++;
    }
    min_active_slot_ = i;
  }

  // should be called from locked state.
  void clear_accepted_entries(){
    for(int i = max_committed_slot_; i <= max_accepted_slot_; i++){
      Log_info("Erasing entry number %ld", i);
      logs_.erase(i);
    }
  }

  virtual bool HandleConflicts(Tx& dtxn,
                               innid_t inn_id,
                               vector<string>& conflicts) {
    verify(0);
  };
};
} // namespace janus
