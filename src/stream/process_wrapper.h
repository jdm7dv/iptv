/*  Copyright (C) 2014-2018 FastoGT. All right reserved.
    This file is part of iptv_cloud.
    iptv_cloud is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    iptv_cloud is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with iptv_cloud.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <thread>

#include <common/libev/io_loop.h>
#include <common/libev/io_loop_observer.h>
#include <common/threads/barrier.h>

#include "protocol/types.h"

#include "utils/utils.h"

#include "stream/ibase_stream.h"

namespace iptv_cloud {
namespace stream {

class ProcessWrapper : public common::libev::IoLoopObserver, public IBaseStream::IStreamClient {
 public:
  enum constants : uint32_t { restart_attempts = 10, restart_after_frozen_sec = 60 };

  ProcessWrapper(const std::string& process_name,
                 const std::string& feedback_dir,
                 const utils::ArgsMap& config_args,
                 common::libev::IoClient* command_client,
                 StreamStruct* mem);
  ~ProcessWrapper() override;

  int Exec();

 protected:
  virtual common::ErrnoError HandleRequestCommand(common::libev::IoClient* client,
                                                  protocol::request_t* req) WARN_UNUSED_RESULT;
  virtual common::ErrnoError HandleResponceCommand(common::libev::IoClient* client,
                                                   protocol::responce_t* resp) WARN_UNUSED_RESULT;

 private:
  protocol::sequance_id_t NextRequestID();

  common::ErrnoError HandleRequestStopStream(common::libev::IoClient* client,
                                             protocol::request_t* req) WARN_UNUSED_RESULT;
  common::ErrnoError HandleRequestRestartStream(common::libev::IoClient* client,
                                                protocol::request_t* req) WARN_UNUSED_RESULT;

  void Stop();
  void Restart();

  void PreLooped(common::libev::IoLoop* loop) override;
  void PostLooped(common::libev::IoLoop* loop) override;
  void Accepted(common::libev::IoClient* client) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoClient* client) override;
  void Closed(common::libev::IoClient* client) override;

  void TimerEmited(common::libev::IoLoop* loop, common::libev::timer_id_t id) override;

  void Accepted(common::libev::IoChild* child) override;
  void Moved(common::libev::IoLoop* server, common::libev::IoChild* child) override;
  void ChildStatusChanged(common::libev::IoChild* child, int status) override;

  void DataReceived(common::libev::IoClient* client) override;
  void DataReadyToWrite(common::libev::IoClient* client) override;

  common::ErrnoError StreamDataRecived(common::libev::IoClient* client) WARN_UNUSED_RESULT;
  void StopStream();
  void RestartStream();

  void OnStatusChanged(IBaseStream* stream, StreamStatus status) override;
  GstPadProbeInfo* OnCheckReveivedData(IBaseStream* stream, Probe* probe, GstPadProbeInfo* info) override;
  GstPadProbeInfo* OnCheckReveivedOutputData(IBaseStream* stream, Probe* probe, GstPadProbeInfo* info) override;
  void OnProbeEvent(IBaseStream* stream, Probe* probe, GstEvent* event) override;
  void OnPipelineEOS(IBaseStream* stream) override;
  void OnTimeoutUpdated(IBaseStream* stream) override;
  void OnSyncMessageReceived(IBaseStream* stream, GstMessage* message) override;
  void OnASyncMessageReceived(IBaseStream* stream, GstMessage* message) override;
  void OnInputChanged(const InputUri& uri) override;

  void OnPipelineCreated(IBaseStream* stream) override;

  common::ErrnoError SendResponceToParent(const std::string& cmd) WARN_UNUSED_RESULT;

  void DumpStreamStatus(StreamStruct* stat, StreamStatus st);

  size_t max_restart_attempts_;
  const std::string process_name_;
  const std::string feedback_dir_;
  const utils::ArgsMap config_args_;
  size_t restart_attempts_;

  std::mutex stop_mutex_;
  std::condition_variable stop_cond_;
  bool stop_;
  std::string channel_id_;

  std::thread ev_thread_;
  common::libev::IoLoop* loop_;
  common::libev::timer_id_t ttl_master_timer_;
  time_t ttl_sec_;
  common::threads::barrier libev_stated_;

  StreamStruct* mem_;

  //
  IBaseStream* origin_;

  std::atomic<protocol::seq_id_t> id_;
};

}  // namespace stream
}  // namespace iptv_cloud
