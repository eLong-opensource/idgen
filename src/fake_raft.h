#ifndef IDGEN_FAKE_RAFT_H
#define IDGEN_FAKE_RAFT_H

#include <raft/core/raft.h>

class FakeRaft : public raft::Raft
{
 public:
  FakeRaft() :logid_(0){}
  virtual ~FakeRaft() {}
  virtual int LeaderId() { return 0; }
  virtual int Broadcast(const std::string& data) {
    syncedCallback_(logid_, data);
    logid_++;
    return 0;
  }
  virtual void SetSyncedCallback(const raft::SyncedCallback& cb) { syncedCallback_ = cb; }
  virtual void SetStateChangedCallback(const raft::StateChangedCallback& cb) { stateChangedCallback_ = cb; }
  virtual void SetTakeSnapshotCallback(const raft::TakeSnapshotCallback& cb) { takeSnapshotCallback_ = cb; }
  virtual void SetLoadSnapshotCallback(const raft::LoadSnapshotCallback& cb) { loadSnapshotCallback_ = cb; }

  void Start() {
    loadSnapshotCallback_(logid_);
    stateChangedCallback_(raft::LEADER);
  }

 private:
  raft::SyncedCallback syncedCallback_;
  raft::StateChangedCallback stateChangedCallback_;
  raft::TakeSnapshotCallback takeSnapshotCallback_;
  raft::LoadSnapshotCallback loadSnapshotCallback_;
  uint64_t logid_;
};

#endif
