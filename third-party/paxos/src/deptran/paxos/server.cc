

#include "server.h"
#include "../paxos_worker.h"
#include "exec.h"
#include "commo.h"

namespace janus {

shared_ptr<ElectionState> es = ElectionState::instance();

void PaxosServer::OnPrepare(slotid_t slot_id,
                            ballot_t ballot,
                            ballot_t *max_ballot,
                            const function<void()> &cb) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("multi-paxos scheduler receives prepare for slot_id: %llx",
            slot_id);
  auto instance = GetInstance(slot_id);
  verify(ballot != instance->max_ballot_seen_);
  if (instance->max_ballot_seen_ < ballot) {
    instance->max_ballot_seen_ = ballot;
  } else {
    // TODO if accepted anything, return;
    verify(0);
  }
  *max_ballot = instance->max_ballot_seen_;
  n_prepare_++;
  cb();
}

void PaxosServer::OnAccept(const slotid_t slot_id,
                           const ballot_t ballot,
                           shared_ptr<Marshallable> &cmd,
                           ballot_t *max_ballot,
                           const function<void()> &cb) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("multi-paxos scheduler accept for slot_id: %llx", slot_id);
  auto instance = GetInstance(slot_id);
  verify(instance->max_ballot_accepted_ < ballot);
  if (instance->max_ballot_seen_ <= ballot) {
    instance->max_ballot_seen_ = ballot;
    instance->max_ballot_accepted_ = ballot;
  } else {
    // TODO
    verify(0);
  }
  *max_ballot = instance->max_ballot_seen_;
  n_accept_++;
  cb();
}

void PaxosServer::OnCommit(const slotid_t slot_id,
                           const ballot_t ballot,
                           shared_ptr<Marshallable> &cmd) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("multi-paxos scheduler decide for slot: %lx", slot_id);
  auto instance = GetInstance(slot_id);
  instance->committed_cmd_ = cmd;
  if (slot_id > max_committed_slot_) {
    max_committed_slot_ = slot_id;
  }
  verify(slot_id > max_executed_slot_);
  for (slotid_t id = max_executed_slot_ + 1; id <= max_committed_slot_; id++) {
    auto next_instance = GetInstance(id);
    if (next_instance->committed_cmd_) {
      app_next_(*next_instance->committed_cmd_);
      Log_debug("multi-paxos par:%d loc:%d executed slot %lx now", partition_id_, loc_id_, id);
      max_executed_slot_++;
      n_commit_++;
    } else {
      break;
    }
  }
  FreeSlots();
}
// marker:ansh change the args to accomodate objects
// marker:ansh add a suitable reply at bottom
void PaxosServer::OnBulkPrepare(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               const function<void()> &cb) {


  auto bp_log = dynamic_pointer_cast<BulkPrepareLog>(cmd);
  es->state_lock();
  if(bp_log->epoch < es->cur_epoch){
    //es->state_unlock();
    *valid = 0;
    *ballot = es->cur_epoch;
    es->state_unlock();
    cb();
    return;
  }

  if(bp_log->epoch == es->cur_epoch && bp_log->leader_id != es->machine_id){
    //es->state_unlock();
    *valid = 0;
    *ballot = es->cur_epoch;
    es->state_unlock();
    cb();
    return;
  }

  /* acquire all other server locks one by one */
  Log_info("Paxos workers size %d %d %d", pxs_workers_g.size(), bp_log->leader_id, bp_log->epoch, es->cur_epoch);
  for(int i = 0; i < bp_log->min_prepared_slots.size(); i++){
    //if(pxs_workers_g[i])
    //	Log_info("cast successfull %d", i);
    PaxosServer* ps = (PaxosServer*)(pxs_workers_g[i]->rep_sched_);
    ps->mtx_.lock();
  }

  /*verify possibility before modification*/
  for(int i = 0; i < bp_log->min_prepared_slots.size(); i++){
    slotid_t slot_id_min = bp_log->min_prepared_slots[i].second;
    PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    BulkPrepare* bp = &ps->bulk_prepares[make_pair(ps->cur_min_prepared_slot_, ps->max_possible_slot_)];
    if(ps->bulk_prepares.size() != 0 && bp->seen_ballot > bp_log->epoch){
      verify(0); // should not happen, should have been caught bp_log->epoch.
    } else{
      // debug
      //if(slot_id_min <= ps->max_committed_slot_){
      //  verify(0); // marker:ansh to handle. // handle later
      //}
    }
  }

  for(int i = 0; i < bp_log->min_prepared_slots.size(); i++){
    slotid_t slot_id_min = bp_log->min_prepared_slots[i].second;
    PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    BulkPrepare* bp = &ps->bulk_prepares[make_pair(ps->cur_min_prepared_slot_, ps->max_possible_slot_)];
    ps->bulk_prepares.erase(make_pair(ps->cur_min_prepared_slot_, ps->max_possible_slot_));
    bp->seen_ballot = bp_log->epoch;
    bp->leader_id = bp_log->leader_id;
    ps->bulk_prepares[make_pair(slot_id_min, max_possible_slot_)] = *bp;
    ps->cur_min_prepared_slot_ = slot_id_min;
    ps->cur_epoch = bp_log->epoch;
    // ps->clear_accepted_entries(); // pending bulk-prepare-return
  }

unlock_and_return:

  /* change election state holder */
  if(es->machine_id != bp_log->leader_id)
    es->set_state(0);
  es->set_leader(bp_log->leader_id);
  es->set_lastseen();
  Log_info("Leader set to %d", bp_log->leader_id);
  es->set_epoch(bp_log->epoch);

  for(int i = 0; i < bp_log->min_prepared_slots.size(); i++){
    PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    ps->mtx_.unlock();
  }
  *ballot = es->cur_epoch;
  es->state_unlock();
  Log_debug("BulkPrepare: Terminating RPC here");
  *valid = 1;
  cb();
}

void PaxosServer::OnHeartbeat(shared_ptr<Marshallable> &cmd,
                              i32* ballot,
                              i32* valid,
                              const function<void()> &cb){

  auto hb_log = dynamic_pointer_cast<HeartBeatLog>(cmd);
  es->state_lock();
  if(hb_log->epoch < es->cur_epoch){
    es->state_unlock();
    *valid = 0;
    *ballot = es->cur_epoch;
    cb();
    return;
  }
  if(hb_log->leader_id == 1 && es->machine_id == 2)
  Log_debug("OnHeartbeat: received heartbeat from machine is %d %d", hb_log->leader_id, es->leader_id);
  if(hb_log->epoch == es->cur_epoch){
    if(hb_log->leader_id != es->leader_id){
      Log_info("Req leader is %d while machine leader is %d", hb_log->leader_id, es->leader_id);
      es->state_unlock();
      verify(0); // should not happen, means there are two leaders with different in the same epoch.
    } else if(hb_log->leader_id == es->leader_id){
      if(hb_log->leader_id != es->machine_id)
        es->set_state(0);
      es->set_epoch(hb_log->epoch);
      es->set_lastseen();
      es->state_unlock();
      *valid = 1;
       cb();
       return;
    } else{
      es->set_lastseen();
      es->state_unlock();
      *valid = 1;
      cb();
      return;
    }
  } else{
    // in this case reply needs to be that it needs a prepare.
    *valid = 2 + es->machine_id;    // hacky way.
    es->set_state(0);
    es->set_epoch(hb_log->epoch);
    for(int i = 0; i < pxs_workers_g.size()-1; i++){
      PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
      ps->mtx_.lock();
      ps->cur_epoch = hb_log->epoch;
      ps->leader_id = hb_log->leader_id;
      ps->mtx_.unlock();
    }
    es->set_lastseen();
    es->set_leader(hb_log->leader_id);
    es->state_unlock();
    *valid = 1; 
    cb();
    return;
  }
}

void PaxosServer::OnCrpcHeartbeat(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state){
  // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp0, with state.size():%d", state.size());
  if (addrChain.size() == 1)
    {
        Log_info("==== OnCrpcHeartbeat reached the final link in the chain");
        auto x = (MultiPaxosCommo *)(this->commo_);
        verify(x->cRPCEvents.find(id) != x->cRPCEvents.end()); // #profile - 1.40%
        // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 2 @ %d", gettid());
        auto ev = x->cRPCEvents[id]; // imagine this to be a pair
        x->cRPCEvents.erase(id);

        // Log_info("==== inside demoserviceimpl::cRPC; results state is following");
        // auto st = dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_);   // #profile - 0.54%
        for (auto el : state)
        {
          // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 3 @ %d", gettid());
          ev.first(el.ballot, el.valid);
          // bool y = ((el.followerAppendOK == 1) && (this->IsLeader()) && (currentTerm == el.followerCurrentTerm));
          ev.second->FeedResponse(el.valid);
        }
        
        Log_info("==== OnCrpcHeartbeat returning from cRPC");
        return;
    }

    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp1");
    // Log_info("calling dynamic_pointer_cast<AppendEntriesCommand>(state.sp_data_)");
    // auto c = dynamic_pointer_cast<AppendEntriesCommand>(cmd.sp_data_);
    // Log_info("return dynamic_pointer_cast<AppendEntriesCommand>(state.sp_data_)");
    BalValResult res;
    auto r = Coroutine::CreateRun([&]()
                                  { this->OnHeartbeat(const_cast<MarshallDeputy&>(cmd).sp_data_,
                                                      &res.ballot,
                                                      &res.valid,
                                                      []() {}); }); // #profile - 2.88%
    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp2, with ballot: %d, and valid: %d", res.ballot, res.valid);
    std::vector<BalValResult> st(state);
    // auto st = dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_);  // #profile - 1.23%  ==> dont think can do anything about it
    // Log_info("returned dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_)");
    st.push_back(res);
    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp3; current size of addrChain is: %d", addrChain.size());
    vector<uint16_t> addrChainCopy(addrChain.begin() + 1, addrChain.end());
    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp4");
    // auto addrChainCopy = addrChain;
    // addrChainCopy.erase(addrChainCopy.begin());
    // Log_info("inside FpgaRaftServer::OnCRPC3; calling CrpcAppendEntries3");
    // Log_info("*** inside FpgaRaftServer::OnCRPC; cp 2 tid: %d", gettid());
    parid_t par_id = this->frame_->site_info_->partition_id_;
    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp5; par_id: %d", par_id);
    ((MultiPaxosCommo *)(this->commo_))->CrpcHeartbeat(par_id, id,
                                                          cmd, addrChainCopy, st);

    // Log_info("**** inside PaxosServer::OnCrpcHeartbeat cp6");
                                                                                              // Log_info("==== returning from void FpgaRaftServer::OnCRPC");
    // Log_info("*** inside FpgaRaftServer::OnCRPC; cp 3 tid: %d", gettid());
}

void PaxosServer::OnBulkPrepare2(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               shared_ptr<BulkPaxosCmd> ret_cmd,
                               const function<void()> &cb){
  pthread_setname_np(pthread_self(), "Follower server thread");
  auto bcmd = dynamic_pointer_cast<PaxosPrepCmd>(cmd);
  ballot_t cur_b = bcmd->ballots[0];
  slotid_t cur_slot = bcmd->slots[0];
  int req_leader = bcmd->leader_id;
  if(req_leader == 1 && es->machine_id != 1)
	Log_debug("Prepare Received from new leader");
  //Log_info("Received paxos Prepare for slot %d ballot %d machine %d",cur_slot, cur_b, req_leader);
  *valid = 1;
  //cb();
  //return;
  auto rbcmd = make_shared<BulkPaxosCmd>();
  Log_debug("Received paxos Prepare for slot %d ballot %d machine %d",cur_slot, cur_b, req_leader);
  //es->state_lock();
  mtx_.lock();
  if(cur_b < cur_epoch){
    *ballot = cur_epoch;
    //es->state_unlock();
    *valid = 0;
    mtx_.unlock();
    cb();
    return;
  }
  mtx_.unlock();

  es->state_lock();
  es->set_lastseen();
  if(req_leader != es->machine_id)
	es->set_state(0);
  es->state_unlock();

  mtx_.lock();
  max_touched_slot = max(max_touched_slot, cur_slot);
  if(cur_b > cur_epoch){
    mtx_.unlock();
    es->state_lock();
    es->set_epoch(cur_b);
    es->set_leader(req_leader); // marker:ansh send leader in every request.
    for(int i = 0; i < pxs_workers_g.size()-1; i++){
      PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
      ps->mtx_.lock();
      ps->cur_epoch = cur_b;
      ps->leader_id = req_leader;
      ps->mtx_.unlock();
    }
    es->state_unlock();
  } else{
    mtx_.unlock();
    if(req_leader != es->leader_id){ // kshivam: triggered in one of the crpc runs, how?
      Log_info("Req leader is %d and prev leader is %d", req_leader, es->leader_id);
      verify(0); //more than one leader in a term, should not send prepare if not leader.
    }
  }

  mtx_.lock();
  auto instance = GetInstance(cur_slot);
  Log_debug("OnBulkPrepare2: Checks successfull preparing response for slot %d %d", cur_slot, partition_id_);
  if(!instance || !instance->accepted_cmd_){
    mtx_.unlock();
    *valid = 2;
    *ballot = cur_b;
    //*ret_cmd = *bcmd;
    // ret_cmd->ballots.push_back(bcmd->ballots[0]);
    // ret_cmd->slots.push_back(bcmd->slots[0]);
    // ret_cmd->cmds.push_back(bcmd->cmds[0]);
    //Log_info("OnBulkPrepare2: the kind_ of the response object is");
    //es->state_unlock();
    cb();
    //es->state_unlock();
    return;
  }
  //es->state_unlock();
  Log_debug("OnBulkPrepare2: instance found, Preparing response");
  ret_cmd->ballots.push_back(instance->max_ballot_accepted_);
  ret_cmd->slots.push_back(cur_slot);
  ret_cmd->cmds.push_back(make_shared<MarshallDeputy>(instance->accepted_cmd_));
  mtx_.unlock();
  cb();
}

void PaxosServer::OnSyncLog(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               shared_ptr<SyncLogResponse> ret_cmd,
                               const function<void()> &cb){
  auto bcmd = dynamic_pointer_cast<SyncLogRequest>(cmd);
  es->state_lock();
  if(bcmd->epoch < es->cur_epoch){
    //es->state_unlock();
    *valid = 0;
    *ballot = es->cur_epoch;
    es->state_unlock();
    cb();
    return;
  }
  es->state_unlock();
  *valid = 1;
  for(int i = 0; i < pxs_workers_g.size()-1; i++){
    ret_cmd->missing_slots.push_back(vector<slotid_t>{});
    PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    auto bp_cmd = make_shared<BulkPaxosCmd>();
    ps->mtx_.lock();

    for(int j = bcmd->sync_commit_slot[i]; j <= ps->max_committed_slot_; j++){
      auto inst = ps->GetInstance(j);
      if(inst->committed_cmd_){
        bp_cmd->slots.push_back(j);
        bp_cmd->ballots.push_back(inst->max_ballot_accepted_);
        auto temp_cmd = inst->committed_cmd_;
      	MarshallDeputy md(temp_cmd);
      	auto shrd_ptr = make_shared<MarshallDeputy>(md);
        bp_cmd->cmds.push_back(shrd_ptr);
      }
    }
    Log_info("The partition %d, and max executed slot is %d and sync commit is %d", i, ps->max_executed_slot_, bcmd->sync_commit_slot[i]);
    for(int j = ps->max_executed_slot_; j < bcmd->sync_commit_slot[i]; j++){
      auto inst = ps->GetInstance(j);
      if(!inst->committed_cmd_){
        ret_cmd->missing_slots[i].push_back(j);
      }
    }
    Log_info("The partition %d has missing slots size %d", i, ret_cmd->missing_slots[i].size());
    auto sp_marshallable = dynamic_pointer_cast<Marshallable>(bp_cmd);
    MarshallDeputy bp_md_cmd(sp_marshallable);
    auto bp_sp_md = make_shared<MarshallDeputy>(bp_md_cmd);
    ret_cmd->sync_data.push_back(bp_sp_md);
    ps->mtx_.unlock();
  }
  cb();
}

void PaxosServer::OnBulkAccept(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               const function<void()> &cb) {
  //Log_info("here accept");
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto bcmd = dynamic_pointer_cast<BulkPaxosCmd>(cmd);
  *valid = 1;
  ballot_t cur_b = bcmd->ballots[0];
  slotid_t cur_slot = bcmd->slots[0];
  int req_leader = bcmd->leader_id;
  if(req_leader == 1 && es->machine_id != 1)
        Log_debug("Accept Received from new leader");

  // Log_info("**** OnBulkAccept; current epoch is: %d", cur_epoch);
  //Log_debug("multi-paxos scheduler accept for slot: %lx", bcmd->slots.size());
  //es->state_lock();
  mtx_.lock();
  if(cur_b < cur_epoch){
    *ballot = cur_epoch;
    //es->state_unlock();
    *valid = 0;
    mtx_.unlock();
    cb();
    return;
  }
  mtx_.unlock();
  es->state_lock();
  es->set_lastseen();
  if(req_leader != es->machine_id)
  es->set_state(0);
  es->state_unlock();
  //cb();
  //return;
  //Log_info("multi-paxos scheduler accept for slot: %ld, par_id: %d", cur_slot, partition_id_);
  for(int i = 0; i < bcmd->slots.size(); i++){
      slotid_t slot_id = bcmd->slots[i];
      ballot_t ballot_id = bcmd->ballots[i];
      // Log_info("**** OnBulkAccept; ballot_id:%d", ballot_id);
      mtx_.lock();
      if(cur_epoch > ballot_id){
        *valid = 0;
        *ballot = cur_epoch;
        mtx_.unlock();
        break;
      } else{
        if(cur_epoch < ballot_id){
          mtx_.unlock();
          //Log_info("I am here");
          for(int i = 0; i < pxs_workers_g.size()-1; i++){
            PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
            ps->mtx_.lock();
            ps->cur_epoch = ballot_id;
            ps->leader_id = req_leader;
            ps->mtx_.unlock();
          }
        } else{
          mtx_.unlock();
        }
        es->state_lock();
        es->set_leader(req_leader);
        es->state_unlock();
        auto instance = GetInstance(slot_id);
        //verify(instance->max_ballot_accepted_ < ballot_id);
        instance->max_ballot_seen_ = ballot_id;
        instance->max_ballot_accepted_ = ballot_id;
        instance->accepted_cmd_ = bcmd->cmds[i].get()->sp_data_;
        max_accepted_slot_ = slot_id;
        n_accept_++;
        *valid &= 1;
	      *ballot = ballot_id;
      }
  }
  if(req_leader != 0)
	Log_debug("multi-paxos scheduler accept for slot: %ld, par_id: %d", cur_slot, partition_id_);  
  //es->state_unlock();
  cb();
  //Log_info("multi-paxos scheduler accept for slot: %ld, par_id: %d", cur_slot, partition_id_);
}


void PaxosServer::OnCrpcBulkAccept(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state){
  // Log_info("**** inside PaxosServer::CrpcBulkAccept cp0, with state.size():%d", state.size());
  Log_info("inside paxosServer::OnCrpcBulkAccept, with crpc_id: %ld", id);  
  if (addrChain.size() == 1)
    {
        Log_info("inside paxosServer::OnCrpcBulkAccept, inside chain , with crpc_id: %ld", id);  
        Log_info("**** CrpcBulkAccept reached the final link in the chain with par_id: %d", this->frame_->site_info_->partition_id_);
        auto x = (MultiPaxosCommo *)(this->commo_);
        verify(x->cRPCEvents.find(id) != x->cRPCEvents.end()); // #profile - 1.40%
        // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 2 @ %d", gettid());
        auto ev = x->cRPCEvents[id]; // imagine this to be a pair
        x->cRPCEvents.erase(id);
        // Log_info("**** inside the last link in the chain; res.ballot:%d, res.valid:%d", res.ballot, res.valid);
        // Log_info("==== inside demoserviceimpl::cRPC; results state is following");
        // auto st = dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_);   // #profile - 0.54%
        for (auto el : state)
        {
          // Log_info("**** inside the last link in the chain; el.ballot:%d, el.valid:%d", el.ballot, el.valid);
          // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 3 @ %d", gettid());
          ev.first(el.ballot, el.valid);
          // bool y = ((el.followerAppendOK == 1) && (this->IsLeader()) && (currentTerm == el.followerCurrentTerm));
          ev.second->FeedResponse(el.valid);
        }
        
        // Log_info("**** CrpcBulkAccept returning from cRPC");
        return;
    }
    BalValResult res;
    Log_info("***** calling onBulkAccept, with crpc_id: %ld", id);
    auto r = Coroutine::CreateRun([&]()
                                  { this->OnBulkAccept(const_cast<MarshallDeputy&>(cmd).sp_data_,
                                                      &res.ballot,
                                                      &res.valid,
                                                      []() {}); }); // #profile - 2.88%
    Log_info("***** returned from calling onBulkAccept, with crpc_id: %ld", id);
    // Log_info("**** inside PaxosServer::CrpcBulkAccept cp1");
    // Log_info("calling dynamic_pointer_cast<AppendEntriesCommand>(state.sp_data_)");
    // auto c = dynamic_pointer_cast<AppendEntriesCommand>(cmd.sp_data_);
    // Log_info("return dynamic_pointer_cast<AppendEntriesCommand>(state.sp_data_)");
    // BalValResult res;
    // auto r = Coroutine::CreateRun([&]()
    //                               { this->OnBulkAccept(const_cast<MarshallDeputy&>(cmd).sp_data_,
    //                                                   &res.ballot,
    //                                                   &res.valid,
    //                                                   []() {}); }); // #profile - 2.88%
    // Log_info("**** inside PaxosServer::CrpcBulkAccept cp2, with ballot: %d, and valid: %d", res.ballot, res.valid);
    std::vector<BalValResult> st(state);
    // auto st = dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_);  // #profile - 1.23%  ==> dont think can do anything about it
    // Log_info("returned dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_)");
    st.push_back(res);
    // Log_info("**** inside PaxosServer::CrpcBulkAccept cp3; current size of addrChain is: %d", addrChain.size());
    vector<uint16_t> addrChainCopy(addrChain.begin() + 1, addrChain.end());
    // Log_info("**** inside PaxosServer::CrpcBulkAccept cp4");

    parid_t par_id = this->frame_->site_info_->partition_id_;
    // kshivam todo: maybe remove this optimization; adding an optimization to not 
    // send the cmd data over the network to save b/w
    // if (false){
    if (addrChainCopy.size() == 1){
      auto sp_cmd = make_shared<LogEntry>();
      // auto sp_m = dynamic_pointer_cast<Marshallable>(sp_cmd);
      Log_info("***** checkpoint check check check with crpc_id: %ld", id);
      MarshallDeputy ph(sp_cmd);
      ((MultiPaxosCommo *)(this->commo_))->CrpcBulkAccept(par_id, id,
                                                          ph, addrChainCopy, st);
    }
    else{
    Log_info("**** inside PaxosServer::CrpcBulkAccept cp5; par_id: %d", par_id);
    if (!this->commo_){
      Log_info("***** commo is not initialized with crpc_id: %ld", id);
    }
    ((MultiPaxosCommo *)(this->commo_))->CrpcBulkAccept(par_id, id,
                                                          cmd, addrChainCopy, st);
    }
    Log_info("**** inside PaxosServer::CrpcBulkAccept cp6 with crpc_id: %ld", id);
}

void PaxosServer::OnSyncCommit(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               const function<void()> &cb) {
  //Log_info("here");
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  //mtx_.lock();
  //Log_info("here");
  //Log_info("multi-paxos scheduler decide for slot: %ld", bcmd->slots.size());
  auto bcmd = dynamic_pointer_cast<BulkPaxosCmd>(cmd);
  *valid = 1;
  ballot_t cur_b = bcmd->ballots[0];
  slotid_t cur_slot = bcmd->slots[0];
  //Log_info("multi-paxos scheduler decide for slot: %ld", cur_slot);
  int req_leader = bcmd->leader_id;
  //es->state_lock();
  mtx_.lock();
  if(cur_b < cur_epoch){
    *ballot = cur_epoch;
    //es->state_unlock();
    *valid = 0;
    mtx_.unlock();
    cb();
    return;
  }
  mtx_.unlock();
  es->state_lock();
  es->set_lastseen();
  if(req_leader != es->machine_id)
  es->set_state(0);
  es->state_unlock();
  vector<shared_ptr<PaxosData>> commit_exec;
  for(int i = 0; i < bcmd->slots.size(); i++){
      //break;
      slotid_t slot_id = bcmd->slots[i];
      ballot_t ballot_id = bcmd->ballots[i];
      mtx_.lock();
      if(cur_epoch > ballot_id){
        *valid = 0;
        *ballot = cur_epoch;
        mtx_.unlock();
        break;
      } else{
        if(cur_epoch < ballot_id){
          mtx_.unlock();
          for(int i = 0; i < pxs_workers_g.size()-1; i++){
            PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
            ps->mtx_.lock();
            ps->cur_epoch = ballot_id;
            ps->leader_id = req_leader;
            ps->mtx_.unlock();
          }
        } else{
          mtx_.unlock();
        }
        es->state_lock();
        es->set_leader(req_leader);
        es->state_unlock();
        
        auto instance = GetInstance(slot_id);
        verify(instance->max_ballot_accepted_ <= ballot_id);
        instance->max_ballot_seen_ = ballot_id;
        instance->max_ballot_accepted_ = ballot_id;
        instance->committed_cmd_ = bcmd->cmds[i].get()->sp_data_;
        *valid &= 1;
        if (slot_id > max_committed_slot_) {
            max_committed_slot_ = slot_id;
        }
        
      }
  }
  //es->state_unlock();
  if(*valid == 0){
    cb();
    return;
  }
  //mtx_.lock();
  //Log_info("The commit batch size is %d", bcmd->slots.size());
  for (slotid_t id = max_executed_slot_ + 1; id <= max_committed_slot_; id++) {
      //break;
      auto next_instance = GetInstance(id);
      if (next_instance->committed_cmd_) {
          //app_next_(*next_instance->committed_cmd_);
	        commit_exec.push_back(next_instance);
	        //Log_debug("multi-paxos par:%d loc:%d executed slot %lx now", partition_id_, loc_id_, id);
          max_executed_slot_++;
          n_commit_++;
      } else {
          break;
      }
   } 
  //mtx_.unlock();
  //Log_info("Committing %d", commit_exec.size());
  for(int i = 0; i < commit_exec.size(); i++){
      //auto x = new PaxosData();
      app_next_(*commit_exec[i]->committed_cmd_);
  }

  *valid = 1;
  //cb();

  //mtx_.lock();
  //FreeSlots();
  //mtx_.unlock();
  cb();
}

void PaxosServer::OnBulkCommit(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               const function<void()> &cb) {
  //Log_info("here");
  //std::lock_guard<std::recursive_mutex> lock(mtx_);
  //mtx_.lock();
  //Log_info("here");
  //Log_info("multi-paxos scheduler decide for slot: %ld", bcmd->slots.size());
  auto bcmd = dynamic_pointer_cast<PaxosPrepCmd>(cmd);
  *valid = 1;
  ballot_t cur_b = bcmd->ballots[0];
  slotid_t cur_slot = bcmd->slots[0];
  //Log_info("multi-paxos scheduler decide for slot: %ld", cur_slot);
  int req_leader = bcmd->leader_id;
  //es->state_lock();
  mtx_.lock();

  // Log_info("**** OnBulkCommit; current epoch is: %d", cur_epoch);
  if(cur_b < cur_epoch){
    *ballot = cur_epoch;
    //es->state_unlock();
    *valid = 0;
    mtx_.unlock();
    cb();
    return;
  }
  /*if(req_leader != 0 && es->machine_id == 2)
	Log_info("Stuff in getting committed on machine %d", bcmd->slots[0]);
  */
  mtx_.unlock();
  es->state_lock();
  es->set_lastseen();
  if(req_leader != es->machine_id)
  es->set_state(0);
  es->state_unlock();
  vector<shared_ptr<PaxosData>> commit_exec;
  for(int i = 0; i < bcmd->slots.size(); i++){
      //break;
      slotid_t slot_id = bcmd->slots[i];
      ballot_t ballot_id = bcmd->ballots[i];
      // Log_info("**** OnBulkCommit; ballot_id:%d", ballot_id);
      mtx_.lock();
      if(cur_epoch > ballot_id){
        *valid = 0;
        *ballot = cur_epoch;
        mtx_.unlock();
        break;
      } else{
        if(cur_epoch < ballot_id){
          mtx_.unlock();
          for(int i = 0; i < pxs_workers_g.size()-1; i++){
            PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
            ps->mtx_.lock();
            ps->cur_epoch = ballot_id;
            ps->leader_id = req_leader;
            ps->mtx_.unlock();
          }
        } else{
          mtx_.unlock();
        }
        es->state_lock();
        es->set_leader(req_leader);
        es->state_unlock();
        
        auto instance = GetInstance(slot_id);
        // Log_info("**** OnBulkCommit; slot_id: %d, max_ballot_accepted: %d", slot_id, instance->max_ballot_accepted_);
        verify(instance->max_ballot_accepted_ == ballot_id); //todo: for correctness, if a new commit comes, sync accept.
        instance->max_ballot_seen_ = ballot_id;
        instance->max_ballot_accepted_ = ballot_id;
        instance->committed_cmd_ = instance->accepted_cmd_;
        *valid &= 1;
        if (slot_id > max_committed_slot_) {
            max_committed_slot_ = slot_id;
        }
        
      }
  }
  //es->state_unlock();
  if(*valid == 0){
    cb();
    return;
  }
  //mtx_.lock();
  //Log_info("The commit batch size is %d", bcmd->slots.size());
  for (slotid_t id = max_executed_slot_ + 1; id <= max_committed_slot_; id++) {
      //break;
      auto next_instance = GetInstance(id);
      if (next_instance->committed_cmd_) {
          //app_next_(*next_instance->committed_cmd_);
          commit_exec.push_back(next_instance);
          if(req_leader == 0)
		Log_debug("multi-paxos par:%d loc:%d executed slot %ld now", partition_id_, loc_id_, id);
          max_executed_slot_++;
          n_commit_++;
      } else {
	  if(req_leader != 0)
		Log_info("Some slot is stopping commit %d %d and partition %d", id, bcmd->slots[0], partition_id_);
          break;
      }
   }
   if(commit_exec.size() > 0){
	Log_debug("Something is getting committed %d", commit_exec.size());
   } 
  //mtx_.unlock();
  //Log_info("Committing %d", commit_exec.size());
  for(int i = 0; i < commit_exec.size(); i++){
      //auto x = new PaxosData();
      app_next_(*commit_exec[i]->committed_cmd_); // kshivam: Next is invoked here
  }

  *valid = 1;
  //cb();

  //mtx_.lock();
  //FreeSlots();
  //mtx_.unlock();
  cb();

  // Log_info("**** OnBulkCommit; ballot:%d, and valid: %d", *ballot, *valid);
}

void PaxosServer::OnCrpcBulkCommit(const uint64_t& id,
                  const MarshallDeputy& cmd, 
                  const std::vector<uint16_t>& addrChain, 
                  const std::vector<BalValResult>& state){
  // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp0, with state.size():%d", state.size());
  BalValResult res;
  auto r = Coroutine::CreateRun([&]()
                                  { this->OnBulkCommit(const_cast<MarshallDeputy&>(cmd).sp_data_,
                                                      &res.ballot,
                                                      &res.valid,
                                                      []() {}); }); // #profile - 2.88%
  // Log_info("**** OnCrpcBulkCommit; res.ballot:%d, res.valid:%d", res.ballot, res.valid);
  if (addrChain.size() == 1)
    {
        // Log_info("**** OnCrpcBulkCommit reached the final link in the chain");
        auto x = (MultiPaxosCommo *)(this->commo_);
        verify(x->cRPCEvents.find(id) != x->cRPCEvents.end()); // #profile - 1.40%
        // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 2 @ %d", gettid());
        auto ev = x->cRPCEvents[id]; // imagine this to be a pair
        x->cRPCEvents.erase(id);
        
        ev.first(res.ballot, res.valid);
        ev.second->FeedResponse(res.valid);
        // Log_info("==== inside demoserviceimpl::cRPC; results state is following");
        // auto st = dynamic_pointer_cast<AppendEntriesCommandState>(state.sp_data_);   // #profile - 0.54%
        for (auto el : state)
        {
          // Log_info("**** inside the last link in the chain OnCrpcBulkCommit; el.ballot:%d, el.valid:%d", el.ballot, el.valid);
          // Log_info("inside FpgaRaftServer::OnCRPC2; checkpoint 3 @ %d", gettid());
          ev.first(el.ballot, el.valid);
          // bool y = ((el.followerAppendOK == 1) && (this->IsLeader()) && (currentTerm == el.followerCurrentTerm));
          ev.second->FeedResponse(el.valid);
        }
        
        // Log_info("**** OnCrpcBulkCommit returning from cRPC");
        return;
    }
    // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp2, with ballot: %d, and valid: %d", res.ballot, res.valid);
    std::vector<BalValResult> st(state);
    st.push_back(res);
    // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp3; current size of addrChain is: %d", addrChain.size());
    vector<uint16_t> addrChainCopy(addrChain.begin() + 1, addrChain.end());
    // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp4");

    parid_t par_id = this->frame_->site_info_->partition_id_;
    // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp5; par_id: %d", par_id);
    ((MultiPaxosCommo *)(this->commo_))->CrpcBulkDecide(par_id, id,
                                                          cmd, addrChainCopy, st);

    // Log_info("**** inside PaxosServer::OnCrpcBulkCommit cp6");
}


void PaxosServer::OnSyncNoOps(shared_ptr<Marshallable> &cmd,
                               i32* ballot,
                               i32* valid,
                               const function<void()> &cb){

  auto bcmd = dynamic_pointer_cast<SyncNoOpRequest>(cmd);
  *valid = 1;
  ballot_t cur_b = bcmd->epoch;
  int req_leader = bcmd->leader_id;
  //es->state_lock();
  mtx_.lock();
  if(cur_b < cur_epoch){
    *ballot = cur_epoch;
    //es->state_unlock();
    *valid = 0;
    mtx_.unlock();
    cb();
    return;
  }
  mtx_.unlock();

  for(int i = 0; i < pxs_workers_g.size()-1; i++){
    PaxosServer* ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    ps->mtx_.lock();
    if(bcmd->sync_slots[i] <= ps->max_executed_slot_){
      Log_info("The sync slot is %d for partition %d and committed slot is %d", bcmd->sync_slots[i], i, ps->max_executed_slot_);
      verify(0);
    }
    Log_info("NoOps sync slot is %d for partition %d", bcmd->sync_slots[i], i);
    for(int j = bcmd->sync_slots[i]; j <= ps->max_committed_slot_; j++){
      auto instance = ps->GetInstance(j);
      if(instance->committed_cmd_)
	      continue;
      instance->committed_cmd_ = make_shared<LogEntry>();
      instance->is_no_op = true;
      instance->max_ballot_accepted_ = cur_b;
    }
    for (slotid_t id = ps->max_executed_slot_ + 1; id <= ps->max_committed_slot_; id++) {
      auto next_instance = ps->GetInstance(id);
      if (next_instance->committed_cmd_ && !next_instance->is_no_op) {
          ps->app_next_(*next_instance->committed_cmd_);
          ps->max_executed_slot_++;
          ps->n_commit_++;
      } else {
          verify(0);
      }
    }
    ps->max_committed_slot_ = ps->max_committed_slot_;
    ps->max_executed_slot_ = ps->max_committed_slot_;
    ps->cur_open_slot_ = ps->max_committed_slot_+1;
    ps->mtx_.unlock();
  }

  *valid = 1;
  cb();

}

} // namespace janus
