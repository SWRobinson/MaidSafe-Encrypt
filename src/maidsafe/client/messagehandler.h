/*
* ============================================================================
*
* Copyright [2009] maidsafe.net limited
*
* Description:  Handles messages!
* Version:      1.0
* Created:      09/09/2008 12:14:35 PM
* Revision:     none
* Compiler:     gcc
* Author:       David Irvine (di), david.irvine@maidsafe.net
* Company:      maidsafe.net limited
*
* The following source code is property of maidsafe.net limited and is not
* meant for external use.  The use of this code is governed by the license
* file LICENSE.TXT found in the root of this directory and also on
* www.maidsafe.net.
*
* You are not free to copy, amend or otherwise use this source code without
* the explicit written permission of the board of directors of maidsafe.net.
*
* ============================================================================
*/

#ifndef MAIDSAFE_CLIENT_MESSAGEHANDLER_H_
#define MAIDSAFE_CLIENT_MESSAGEHANDLER_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include <string>
#include <vector>

#include "maidsafe/crypto.h"
#include "maidsafe/maidsafe.h"
#include "maidsafe/client/sessionsingleton.h"
#include "maidsafe/client/storemanager.h"
#include "protobuf/datamaps.pb.h"

namespace maidsafe {

const int parallelSendMsgs = 1;

struct Receivers {
  Receivers() : id(""), public_key() {}
  std::string id;
  std::string public_key;
};

struct SendMessagesData {
  SendMessagesData()
      : receivers(),
        no_auth_rec(),
        msg(""),
        index(0),
        successful_stores(0),
        stores_done(0),
        active_sends(0),
        p_type(),
        m_type(),
        cb(),
        is_calledback(false) {}
  std::vector<Receivers> receivers;
  std::vector<std::string> no_auth_rec;
  std::string msg;
  int index;
  int successful_stores;
  int stores_done;
  int active_sends;
  buffer_packet_type p_type;
  packethandler::MessageType m_type;
  base::callback_func_type cb;
  bool is_calledback;
};

class MessageHandler {
 public:
  MessageHandler(StoreManagerInterface *sm, boost::recursive_mutex *mutex);
  void SendMessage(const std::string &msg,
                   const std::vector<Receivers> &receivers,
                   const buffer_packet_type &p_type,
                   const packethandler::MessageType &m_type,
                   base::callback_func_type cb);

 private:
  MessageHandler &operator=(const MessageHandler &) { return *this; }
  MessageHandler(const MessageHandler &);
  std::string CreateMessage(const std::string &msg,
                            const std::string &rec_public_key,
                            const packethandler::MessageType &type,
                            const buffer_packet_type &p_type);
  void CreateSignature(const std::string &buffer_name,
                       const buffer_packet_type &type,
                       std::string *signed_request,
                       std::string *signed_public_key);
  void IterativeStoreMsgs(boost::shared_ptr<SendMessagesData> data);
  void StoreMessage_Callback(const std::string &result, int index,
                             boost::shared_ptr<SendMessagesData> data);
  void StoreMessage(int index,
                    boost::shared_ptr<SendMessagesData> data);
  maidsafe::PacketType PacketHandler_PacketType(const buffer_packet_type &type);
  SessionSingleton *ss_;
  StoreManagerInterface *sm_;
  maidsafe_crypto::Crypto co_;
  boost::recursive_mutex *mutex_;
};

}  // namespace maidsafe

#endif  // MAIDSAFE_CLIENT_MESSAGEHANDLER_H_
