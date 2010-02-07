/*
 * copyright maidsafe.net limited 2008
 * The following source code is property of maidsafe.net limited and
 * is not meant for external use. The use of this code is governed
 * by the license file LICENSE.TXT found in the root of this directory and also
 * on www.maidsafe.net.
 *
 * You are not free to copy, amend or otherwise use this source code without
 * explicit written permission of the board of directors of maidsafe.net
 *
 *  Created on: Nov 13, 2008
 *      Author: Team
 */
#include <boost/filesystem/fstream.hpp>
#include <boost/scoped_ptr.hpp>
#include <maidsafe/signed_kadvalue.pb.h>

#include <vector>

#include "fs/filesystem.h"
#include "maidsafe/chunkstore.h"
#include "maidsafe/client/localstoremanager.h"
#include "maidsafe/client/sessionsingleton.h"
#include "protobuf/maidsafe_messages.pb.h"
#include "protobuf/maidsafe_service_messages.pb.h"

namespace fs = boost::filesystem;

namespace maidsafe {

void ExecuteSuccessCallback(const base::callback_func_type &cb,
                            boost::mutex *mutex) {
  boost::mutex::scoped_lock gaurd(*mutex);
  std::string ser_result;
  GenericResponse result;
  result.set_result(kAck);
  result.SerializeToString(&ser_result);
  cb(ser_result);
}

void ExecuteFailureCallback(const base::callback_func_type &cb,
                            boost::mutex *mutex) {
  boost::mutex::scoped_lock gaurd(*mutex);
  std::string ser_result;
  GenericResponse result;
  result.set_result(kNack);
  result.SerializeToString(&ser_result);
  cb(ser_result);
}

void ExecCallbackVaultInfo(const base::callback_func_type &cb,
                           boost::mutex *mutex) {
  boost::mutex::scoped_lock loch(*mutex);
  VaultCommunication vc;
  vc.set_chunkstore("/home/Smer/ChunkStore");
  vc.set_offered_space(base::random_32bit_uinteger());
  boost::uint32_t fspace = base::random_32bit_uinteger();
  while (fspace >= vc.offered_space())
    fspace = base::random_32bit_uinteger();
  vc.set_free_space(fspace);
  vc.set_ip("127.0.0.1");
  vc.set_port((base::random_32bit_uinteger() % 64512) + 1000);
  vc.set_timestamp(base::get_epoch_time());
  std::string ser_vc;
  vc.SerializeToString(&ser_vc);
  cb(ser_vc);
}

LocalStoreManager::LocalStoreManager(
    boost::shared_ptr<ChunkStore> client_chunkstore)
        : db_(), vbph_(), mutex_(),
          local_sm_dir_(file_system::FileSystem::LocalStoreManagerDir()),
          client_chunkstore_(client_chunkstore),
          ss_(SessionSingleton::getInstance()) {}

void LocalStoreManager::Init(int, base::callback_func_type cb) {
  try {
    if (!fs::exists(local_sm_dir_ + "/StoreChunks")) {
      fs::create_directories(local_sm_dir_ + "/StoreChunks");
    }
    if (fs::exists(local_sm_dir_ + "/KademilaDb.db")) {
      db_.open(std::string(local_sm_dir_ + "/KademilaDb.db").c_str());
    } else {
      boost::mutex::scoped_lock loch(mutex_);
      db_.open(std::string(local_sm_dir_ + "/KademilaDb.db").c_str());
      db_.execDML("create table network(key text primary key,value text);");
    }
    boost::thread thr(boost::bind(&ExecuteSuccessCallback, cb, &mutex_));
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
    std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
  }
}

void LocalStoreManager::Close(base::callback_func_type cb, bool) {
  try {
    boost::mutex::scoped_lock loch(mutex_);
    db_.close();
    boost::thread thr(boost::bind(&ExecuteSuccessCallback, cb, &mutex_));
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
    std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
  }
}

int LocalStoreManager::LoadChunk(const std::string &chunk_name,
                                 std::string *data) {
  return FindAndLoadChunk(chunk_name, data);
}

void LocalStoreManager::StoreChunk(const std::string &chunk_name,
                                   const DirType,
                                   const std::string&) {
#ifdef DEBUG
//  printf("LocalStoreManager::StoreChunk - %s\n",
//          hex_chunk_name.substr(0, 10).c_str());
#endif
  std::string hex_chunk_name(base::EncodeToHex(chunk_name));
  fs::path file_path(local_sm_dir_ + "/StoreChunks");
  file_path = file_path / hex_chunk_name;
  client_chunkstore_->Store(chunk_name, file_path);

  ChunkType type = client_chunkstore_->chunk_type(chunk_name);
  fs::path current = client_chunkstore_->GetChunkPath(chunk_name, type, false);
  try {
    if (!fs::exists(file_path)) {
      fs::copy_file(current, file_path);
    }
  }
  catch(const std::exception &e) {
    printf("%s\n", e.what());
  }
  // Move chunk from Outgoing to Normal.
  ChunkType chunk_type =
      client_chunkstore_->chunk_type(chunk_name);
  ChunkType new_type = chunk_type ^ (kOutgoing | kNormal);
  if (client_chunkstore_->ChangeChunkType(chunk_name, new_type) != 0) {
#ifdef DEBUG
    printf("In LocalStoreManager::SendContent, failed to change chunk type.\n");
#endif
  }
}

int LocalStoreManager::DeleteChunk(const std::string &chunk_name,
                                   const boost::uint64_t &chunk_size,
                                   DirType,
                                   const std::string &) {
  ChunkType chunk_type = client_chunkstore_->chunk_type(chunk_name);
    fs::path chunk_path(client_chunkstore_->GetChunkPath(chunk_name, chunk_type,
                                                         false));
  boost::uint64_t size(chunk_size);
  if (size < 2) {
    if (chunk_type < 0 || chunk_path == fs::path("")) {
#ifdef DEBUG
      printf("In LSM::DeleteChunk, didn't find chunk %s in local chunkstore - "
             "cant delete without valid size\n", HexSubstr(chunk_name).c_str());
#endif
      return kDeleteSizeError;
    }
    try {
      size = fs::file_size(chunk_path);
    }
    catch(const std::exception &e) {
  #ifdef DEBUG
      printf("In LSM::DeleteChunk, didn't find chunk %s in local chunkstore - "
             "can't delete without valid size.\n%s\n",
             HexSubstr(chunk_name).c_str(), e.what());
  #endif
      return kDeleteSizeError;
    }
  }
  ChunkType new_type(chunk_type);
  if (chunk_type >= 0) {
    // Move chunk to TempCache.
    if (chunk_type & kNormal)
      new_type = chunk_type ^ (kNormal | kTempCache);
    else if (chunk_type & kOutgoing)
      new_type = chunk_type ^ (kOutgoing | kTempCache);
    else if (chunk_type & kCache)
      new_type = chunk_type ^ (kCache | kTempCache);
    if (!(new_type < 0) &&
        client_chunkstore_->ChangeChunkType(chunk_name, new_type) != kSuccess) {
  #ifdef DEBUG
      printf("In LSM::DeleteChunk, failed to change chunk type.\n");
  #endif
    }
  }
  return kSuccess;
}

bool LocalStoreManager::KeyUnique(const std::string &key, bool) {
  bool result = false;
  std::string hex_key(base::EncodeToHex(key));
  try {
    boost::mutex::scoped_lock loch(mutex_);
    std::string s = "select * from network where key='" + hex_key;
    s += "';";
    CppSQLite3Query q = db_.execQuery(s.c_str());
    if (q.eof())
      result = true;
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
    std::cerr << e.errorCode() << ":" << e.errorMessage() << std::endl;
    result = false;
  }
  if (result) {
    fs::path file_path(local_sm_dir_ + "/StoreChunks");
    file_path = file_path / hex_key;
    result = (!fs::exists(file_path));
  }
  return result;
}

/*
int LocalStoreManager::DeletePacket(const std::string &hex_key,
                                    const std::string &signature,
                                    const std::string &public_key,
                                    const std::string &signed_public_key,
                                    const ValueType &type,
                                    base::callback_func_type cb) {
  std::string key = base::DecodeFromHex(hex_key);
  crypto::Crypto co;
  co.set_hash_algorithm(crypto::SHA_512);
  if (!co.AsymCheckSig(public_key, signed_public_key, public_key,
      crypto::STRING_STRING)) {
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
    return -2;
  }

  if (!co.AsymCheckSig(co.Hash(
      public_key + signed_public_key + key, "", crypto::STRING_STRING, false),
      signature, public_key, crypto::STRING_STRING)) {
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
    return -3;
  }

  std::string result = "";
  try {
    boost::mutex::scoped_lock loch(mutex_);
    std::string s = "select value from network where key='" + hex_key + "';";
    CppSQLite3Query q = db_.execQuery(s.c_str());
    if (q.eof()) {
      boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
      return -4;
    }
    std::string val = q.fieldValue(static_cast<unsigned int>(0));
    result = base::DecodeFromHex(val);
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
    return -5;
  }

  if (result == "") {
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
    return -6;
  }

  GenericPacket syspacket;
  switch (type) {
    case SYSTEM_PACKET:
        if (!syspacket.ParseFromString(result)) {
          boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
          return -7;
        }
        if (!co.AsymCheckSig(syspacket.data(), syspacket.signature(),
            public_key, crypto::STRING_STRING)) {
          boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
          return -8;
        }
        break;
//    case BUFFER_PACKET:
//        if (!vbph_.ValidateOwnerSignature(public_key, result)) {
//          boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, mutex_));
//          return -9;
//        }
//        break;
//    case BUFFER_PACKET_MESSAGE:
//        if (!vbph_.ValidateOwnerSignature(public_key, result)) {
//          boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, mutex_));
//          return -10;
//        }
//        if (!vbph_.ClearMessages(&result)) {
//          boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, mutex_));
//          return -11;
//        }
//        break;
//    case BUFFER_PACKET_INFO: break;
    case CHUNK_REFERENCE: break;
    case WATCH_LIST: break;
    case DATA: break;
    case PDDIR_SIGNED: break;
    case PDDIR_NOTSIGNED: break;
    default: break;
  }
  try {
    boost::mutex::scoped_lock loch(mutex_);
    CppSQLite3Buffer bufSQL;
    bufSQL.format("delete from network where key=%Q;", hex_key.c_str());
    int nRows = db_.execDML(bufSQL);
    if ( nRows > 0 ) {
      if (type != BUFFER_PACKET_MESSAGE) {
        boost::thread thr(boost::bind(&ExecuteSuccessCallback, cb, &mutex_));
        return 0;
      } else {
        std::string enc_value = base::EncodeToHex(result);
        bufSQL.format("insert into network values ('%s', %Q);",
          hex_key.c_str(), enc_value.c_str());
        db_.execDML(bufSQL);
        boost::thread thr(boost::bind(&ExecuteSuccessCallback, cb, &mutex_));
        return 0;
      }
    } else {
      boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
      return -12;
    }
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
    std::cerr << e.errorCode() << "ddddddd:" << e.errorMessage() << std::endl;
    boost::thread thr(boost::bind(&ExecuteFailureCallback, cb, &mutex_));
    return -13;
  }
}

*/

int LocalStoreManager::LoadPacket(const std::string &packet_name,
                                  std::vector<std::string> *results) {
  return GetValue_FromDB(packet_name, results);
}

void LocalStoreManager::DeletePacket(const std::string &packet_name,
                                     const std::vector<std::string> values,
                                     PacketType system_packet_type,
                                     DirType dir_type,
                                     const std::string &msid,
                                     const VoidFuncOneInt &cb) {
  std::string public_key;
  SigningPublicKey(system_packet_type, dir_type, msid, &public_key);
  if (public_key.empty()) {
    cb(kNoPublicKeyToCheck);
    return;
  }

  crypto::Crypto co;
  for (size_t n = 0; n < values.size(); ++n) {
    kad::SignedValue sv;
    if (sv.ParseFromString(values[n])) {
      if (!co.AsymCheckSig(sv.value(), sv.value_signature(), public_key,
          crypto::STRING_STRING)) {
        cb(kDeletePacketFailure);
        return;
      }
    }
  }
  cb(DeletePacket_DeleteFromDb(packet_name, values, public_key));
}

ReturnCode LocalStoreManager::DeletePacket_DeleteFromDb(
    const std::string &key,
    const std::vector<std::string> &values,
    const std::string &public_key) {
  std::string hex_key(base::EncodeToHex(key));
  boost::mutex::scoped_lock loch(mutex_);
  try {
    std::string s("select value from network where key='" + hex_key + "';");
    CppSQLite3Query q = db_.execQuery(s.c_str());
    if (q.eof()) {
#ifdef DEBUG
      printf("LocalStoreManager::DeletePacket_DeleteFromDb - value not there "
             "anyway.\n");
#endif
      return kSuccess;
    } else {
      kad::SignedValue ksv;
      if (ksv.ParseFromString(q.getStringField(0))) {
        crypto::Crypto co;
        if (!co.AsymCheckSig(ksv.value(), ksv.value_signature(), public_key,
            crypto::STRING_STRING)) {
#ifdef DEBUG
          printf("LocalStoreManager::DeletePacket_DeleteFromDb - current value "
                 "failed validation.\n");
#endif
          return kDeletePacketFailure;
        }
      }
    }
  }
  catch(CppSQLite3Exception &e1) {  // NOLINT (Fraser)
#ifdef DEBUG
    printf("Error(%i): %s\n", e1.errorCode(),  e1.errorMessage());
#endif
    return kStoreManagerError;
  }

  int deleted(values.size());
  for (size_t n = 0; n < values.size(); ++n) {
    try {
      std::string hex_value(base::EncodeToHex(values[0]));
      std::string s("delete from network where key='" + hex_key + "' "
                    "and value='" + hex_value + "';");
      int a = db_.execDML(s.c_str());
      if (a < 2) {
        --deleted;
      } else {
#ifdef DEBUG
        printf("LocalStoreManager::DeletePacket_DeleteFromDb - value not there "
               "anyway.\n");
#endif
        return kDeletePacketFailure;
      }
    }
    catch(CppSQLite3Exception &e2) {  // NOLINT (Fraser)
#ifdef DEBUG
      printf("Error(%i): %s\n", e2.errorCode(),  e2.errorMessage());
#endif
      return kStoreManagerError;
    }
  }

  return kSuccess;
}

void LocalStoreManager::StorePacket(const std::string &packet_name,
                                    const std::string &value,
                                    PacketType system_packet_type,
                                    DirType dir_type,
                                    const std::string& msid,
                                    IfPacketExists if_packet_exists,
                                    const VoidFuncOneInt &cb) {
  std::string public_key;
  kad::SignedValue sv;
  if (sv.ParseFromString(value)) {
    SigningPublicKey(system_packet_type, dir_type, msid, &public_key);
    if (public_key.empty()) {
      cb(kNoPublicKeyToCheck);
      return;
    } else {
      crypto::Crypto co;
      if (!co.AsymCheckSig(sv.value(), sv.value_signature(), public_key,
          crypto::STRING_STRING)) {
        cb(kSendPacketFailure);
        return;
      }
    }
  }

  std::vector<std::string> values;
  int n = GetValue_FromDB(packet_name, &values);
  if (n != kSuccess) {
    cb(kStoreManagerError);
    return;
  }
  if (values.empty()) {
    cb(StorePacket_InsertToDb(packet_name, value, public_key, false));
  } else {
    switch (if_packet_exists) {
      case kDoNothingReturnFailure:
          cb(kSendPacketFailure);
          break;
      case kDoNothingReturnSuccess:
          cb(kSuccess);
          break;
      case kOverwrite:
          cb(StorePacket_InsertToDb(packet_name, value, public_key, false));
          break;
      case kAppend:
          cb(StorePacket_InsertToDb(packet_name, value, public_key, true));
          break;
    }
  }
}

ReturnCode LocalStoreManager::StorePacket_InsertToDb(const std::string &key,
                                                     const std::string &value,
                                                     const std::string &pub_key,
                                                     const bool &append) {
  try {
    if (key.length() != kKeySize) {
      return kIncorrectKeySize;
    }
    std::string hex_key = base::EncodeToHex(key);
    std::string s = "select value from network where key='" + hex_key + "';";
    boost::mutex::scoped_lock loch(mutex_);
    CppSQLite3Query q = db_.execQuery(s.c_str());
    if (!q.eof()) {
      std::string dec_value = base::DecodeFromHex(q.getStringField(0));
      kad::SignedValue sv;
      if (sv.ParseFromString(dec_value)) {
        crypto::Crypto co;
        if (!co.AsymCheckSig(sv.value(), sv.value_signature(), pub_key,
            crypto::STRING_STRING)) {
#ifdef DEBUG
          printf("LocalStoreManager::StorePacket_InsertToDb - "
                 "Signature didn't validate.\n");
#endif
          return kStoreManagerError;
        }
      }
    }

    if (!append) {
      s = "delete from network where key='" + hex_key + "';";
      db_.execDML(s.c_str());
    }

    CppSQLite3Buffer bufSQL;
    std::string hex_value = base::EncodeToHex(value);
    s = "insert into network values ('" + hex_key + "', '" + hex_value + "');";
    int a = db_.execDML(s.c_str());
    if (a != 1) {
#ifdef DEBUG
          printf("LocalStoreManager::StorePacket_InsertToDb - "
                 "Insert failed.\n");
#endif
      return kStoreManagerError;
    }
    return kSuccess;
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
#ifdef DEBUG
    printf("Error(%i): %s\n", e.errorCode(),  e.errorMessage());
#endif
    return kStoreManagerError;
  }
}

void LocalStoreManager::SigningPublicKey(PacketType packet_type,
                                         DirType,
                                         const std::string &,
                                         std::string *public_key) {
  public_key->clear();
  switch (packet_type) {
    case MID:
    case ANMID:
      *public_key = ss_->PublicKey(ANMID);
      break;
    case SMID:
    case ANSMID:
      *public_key = ss_->PublicKey(ANSMID);
      break;
    case TMID:
    case ANTMID:
      *public_key = ss_->PublicKey(ANTMID);
      break;
    case MPID:
    case ANMPID:
      *public_key = ss_->PublicKey(ANMPID);
      break;
      // TODO(Fraser#5#): 2010-01-29 - Uncomment below once auth.cc fixed (MAID
      //                               should be signed by ANMAID, not self)
//    case MAID:
//    case ANMAID:
//      *public_key = ss_->PublicKey(ANMAID);
//      break;
//    case PMID:
//      *public_key = ss_->PublicKey(MAID);
//      break;
    case PMID:
    case MAID:
      *public_key = ss_->PublicKey(MAID);
      break;
// TODO(Dan#5#): 2010-02-03 - Dunno wtf with these as packets
//    case MSID:
//    case PD_DIR:
//      GetChunkSignatureKeys(dir_type, msid, key_id, public_key,
//                            public_key_sig, private_key);
//      break;
//    case BUFFER:
//    case BUFFER_INFO:
//    case BUFFER_MESSAGE:
//      *public_key = ss->PublicKey(MPID);
//      break;
    default:
      break;
  }
}

bool LocalStoreManager::ValidateGenericPacket(std::string ser_gp,
                                              std::string public_key) {
  GenericPacket gp;
  crypto::Crypto co;
  if (!gp.ParseFromString(ser_gp))
    return false;
  if (!co.AsymCheckSig(gp.data(), gp.signature(), public_key,
      crypto::STRING_STRING))
    return false;
  return true;
}

// Buffer packet
int LocalStoreManager::CreateBP() {
  if (ss_->Id(MPID) == "")
    return -666;

  std::string bufferpacketname(BufferPacketName()), ser_packet;
#ifdef DEBUG
  printf("LocalStoreManager::CreateBP - BP chunk(%s).\n",
         HexSubstr(bufferpacketname).c_str());
#endif
  BufferPacket buffer_packet;
  GenericPacket *ser_owner_info = buffer_packet.add_owner_info();
  BufferPacketInfo buffer_packet_info;
  buffer_packet_info.set_owner(ss_->Id(MPID));
  buffer_packet_info.set_ownerpublickey(ss_->PublicKey(MPID));
  buffer_packet_info.set_online(1);
  EndPoint *ep = buffer_packet_info.mutable_ep();
  ep->set_ip("127.0.0.1");
  ep->set_port(12700);
  ser_owner_info->set_data(buffer_packet_info.SerializeAsString());
  crypto::Crypto co;
  ser_owner_info->set_signature(co.AsymSign(ser_owner_info->data(), "",
                                ss_->PrivateKey(MPID), crypto::STRING_STRING));
  buffer_packet.SerializeToString(&ser_packet);
  return FlushDataIntoChunk(bufferpacketname, ser_packet, false);
}

int LocalStoreManager::LoadBPMessages(
    std::list<ValidatedBufferPacketMessage> *messages) {
  if (ss_->Id(MPID) == "")
    return -666;

  std::string bp_in_chunk;
  std::string bufferpacketname(BufferPacketName());
  if (FindAndLoadChunk(bufferpacketname, &bp_in_chunk) != 0) {
#ifdef DEBUG
    printf("LocalStoreManager::LoadBPMessages - Failed to find BP chunk.\n");
#endif
    return -1;
  }
  std::vector<std::string> msgs;
  if (!vbph_.GetMessages(&bp_in_chunk, &msgs)) {
#ifdef DEBUG
    printf("LocalStoreManager::LoadBPMessages - Failed to get messages.\n");
#endif
    return -1;
  }
  messages->clear();
  crypto::Crypto co;
  co.set_symm_algorithm(crypto::AES_256);
  for (size_t n = 0; n < msgs.size(); ++n) {
    ValidatedBufferPacketMessage valid_message;
    if (valid_message.ParseFromString(msgs[n])) {
      std::string aes_key = co.AsymDecrypt(valid_message.index(), "",
                            ss_->PrivateKey(MPID), crypto::STRING_STRING);
      valid_message.set_message(co.SymmDecrypt(valid_message.message(),
                                "", crypto::STRING_STRING, aes_key));
      messages->push_back(valid_message);
    }
  }
  if (FlushDataIntoChunk(bufferpacketname, bp_in_chunk, true) != 0) {
#ifdef DEBUG
    printf("LocalStoreManager::LoadBPMessages - "
           "Failed to flush BP into chunk.\n");
#endif
    return -1;
  }
  return 0;
}

int LocalStoreManager::ModifyBPInfo(const std::string &info) {
  if (ss_->Id(MPID) == "")
    return -666;

  std::string bp_in_chunk;
  std::string bufferpacketname(BufferPacketName()), ser_gp;
  GenericPacket gp;
  gp.set_data(info);
  crypto::Crypto co;
  gp.set_signature(co.AsymSign(gp.data(), "", ss_->PrivateKey(MPID),
                   crypto::STRING_STRING));
  gp.SerializeToString(&ser_gp);
  if (FindAndLoadChunk(bufferpacketname, &bp_in_chunk) != 0) {
#ifdef DEBUG
    printf("LocalStoreManager::ModifyBPInfo - Failed to find BP chunk(%s).\n",
           bufferpacketname.substr(0, 10).c_str());
#endif
    return -1;
  }
  std::string new_bp;
  if (!vbph_.ChangeOwnerInfo(ser_gp, &bp_in_chunk, ss_->PublicKey(MPID))) {
#ifdef DEBUG
    printf("LocalStoreManager::ModifyBPInfo - Failed to change owner info.\n");
#endif
    return -2;
  }
  if (FlushDataIntoChunk(bufferpacketname, bp_in_chunk, true) != 0) {
#ifdef DEBUG
    printf("LocalStoreManager::ModifyBPInfo - Failed to flush BP to chunk.\n");
#endif
    return -3;
  }
  return 0;
}

int LocalStoreManager::AddBPMessage(const std::vector<std::string> &receivers,
                                    const std::string &message,
                                    const MessageType &m_type) {
  if (ss_->Id(MPID) == "")
    return -666;

  std::string bp_in_chunk, ser_gp;
  int fails = 0;
  boost::uint32_t timestamp = base::get_epoch_time();
  for (size_t n = 0; n < receivers.size(); ++n) {
    std::string rec_pub_key(ss_->GetContactPublicKey(receivers[n]));
    std::string bufferpacketname(BufferPacketName(receivers[n], rec_pub_key));
    if (FindAndLoadChunk(bufferpacketname, &bp_in_chunk) != 0) {
#ifdef DEBUG
      printf("LocalStoreManager::AddBPMessage - Failed to find BP chunk (%s)\n",
             receivers[n].c_str());
#endif
      ++fails;
      continue;
    }

    std::string updated_bp;
    if (!vbph_.AddMessage(bp_in_chunk,
        CreateMessage(message, rec_pub_key, m_type, timestamp), "",
        &updated_bp)) {
#ifdef DEBUG
      printf("LocalStoreManager::AddBPMessage - Failed to add message (%s).\n",
             receivers[n].c_str());
#endif
      ++fails;
      continue;
    }

    if (FlushDataIntoChunk(bufferpacketname, updated_bp, true) != 0) {
#ifdef DEBUG
      printf("LocalStoreManager::AddBPMessage - "
             "Failed to flush BP into chunk. (%s).\n",
             receivers[n].c_str());
#endif
      ++fails;
      continue;
    }
  }
  return fails;
}

void LocalStoreManager::ContactInfo(const std::string &public_username,
                                    const std::string &me,
                                    ContactInfoNotifier cin) {
  std::string rec_pub_key(ss_->GetContactPublicKey(public_username));
  std::string bufferpacketname(BufferPacketName(public_username, rec_pub_key));
  std::string bp_in_chunk;
  EndPoint ep;
  boost::uint16_t status(1);
  if (FindAndLoadChunk(bufferpacketname, &bp_in_chunk) != 0) {
    boost::thread thr(cin, kGetBPInfoError, ep, status);
#ifdef DEBUG
    printf("LocalStoreManager::ContactInfo - Failed to find BP chunk(%s).\n",
           bufferpacketname.substr(0, 10).c_str());
#endif
    return;
  }

  if (!vbph_.ContactInfo(bp_in_chunk, me, &ep, &status)) {
    boost::thread thr(cin, kGetBPInfoError, ep, status);
#ifdef DEBUG
    printf("LocalStoreManager::ContactInfo - Failed(%i) to get info (%s).\n",
           kGetBPInfoError, public_username.c_str());
#endif
    return;
  }

  boost::thread thr(cin, kSuccess, ep, status);
}

int LocalStoreManager::FindAndLoadChunk(const std::string &chunkname,
                                        std::string *data) {
  std::string hex_chunkname(base::EncodeToHex(chunkname));
  fs::path file_path(local_sm_dir_ + "/StoreChunks");
  file_path = file_path / hex_chunkname;
  try {
    if (!fs::exists(file_path)) {
#ifdef DEBUG
      printf("LocalStoreManager::FindAndLoadChunk - didn't find the BP.\n");
#endif
      return -1;
    }
    boost::uintmax_t size = fs::file_size(file_path);
    boost::scoped_ptr<char> temp(new char[size]);
    fs::ifstream fstr;
    fstr.open(file_path, std::ios_base::binary);
    fstr.read(temp.get(), size);
    fstr.close();
    *data = std::string((const char*)temp.get(), size);
  }
  catch(const std::exception &e) {
#ifdef DEBUG
    printf("LocalStoreManager::FindAndLoadChunk - %s\n", e.what());
#endif
    return -1;
  }
  return 0;
}

int LocalStoreManager::FlushDataIntoChunk(const std::string &chunkname,
                                          const std::string &data,
                                          const bool &overwrite) {
  std::string hex_chunkname(base::EncodeToHex(chunkname));
  fs::path file_path(local_sm_dir_ + "/StoreChunks");
  file_path = file_path / hex_chunkname;
  try {
    if (boost::filesystem::exists(file_path) && !overwrite) {
#ifdef DEBUG
      printf("This BP (%s) already exists\n.",
             hex_chunkname.substr(0, 10).c_str());
#endif
      return -1;
    }
    boost::filesystem::ofstream bp_file(file_path.string().c_str(),
                                        boost::filesystem::ofstream::binary);
    bp_file << data;
    bp_file.close();
  }
  catch(const std::exception &e) {
#ifdef DEBUG
    printf("%s\n", e.what());
#endif
    return -1;
  }
  return 0;
}

std::string LocalStoreManager::BufferPacketName() {
  return BufferPacketName(ss_->Id(MPID), ss_->PublicKey(MPID));
}

std::string LocalStoreManager::BufferPacketName(const std::string &pub_username,
                                                const std::string &public_key) {
  crypto::Crypto co;
  co.set_hash_algorithm(crypto::SHA_512);
  return co.Hash(pub_username + public_key, "", crypto::STRING_STRING, false);
}

std::string LocalStoreManager::CreateMessage(const std::string &message,
                                             const std::string &rec_public_key,
                                             const MessageType &m_type,
                                             const boost::uint32_t &timestamp) {
  BufferPacketMessage bpm;
  GenericPacket gp;

  bpm.set_sender_id(ss_->Id(MPID));
  bpm.set_sender_public_key(ss_->PublicKey(MPID));
  bpm.set_type(m_type);
  crypto::Crypto co;
  co.set_hash_algorithm(crypto::SHA_512);
  co.set_symm_algorithm(crypto::AES_256);
  int iter = base::random_32bit_uinteger() % 1000 +1;
  std::string aes_key = co.SecurePassword(co.Hash(message, "",
                        crypto::STRING_STRING, true), iter);
  bpm.set_rsaenc_key(co.AsymEncrypt(aes_key, "", rec_public_key,
                                    crypto::STRING_STRING));
  bpm.set_aesenc_message(co.SymmEncrypt(message, "", crypto::STRING_STRING,
                         aes_key));
  bpm.set_timestamp(timestamp);
  std::string ser_bpm;
  bpm.SerializeToString(&ser_bpm);
  gp.set_data(ser_bpm);
  gp.set_signature(co.AsymSign(gp.data(), "", ss_->PrivateKey(MPID),
                   crypto::STRING_STRING));
  std::string ser_gp;
  gp.SerializeToString(&ser_gp);
  return ser_gp;
}

int LocalStoreManager::GetValue_FromDB(const std::string &key,
                                       std::vector<std::string> *results) {
  std::string hex_key = base::EncodeToHex(key);
  try {
    boost::mutex::scoped_lock loch(mutex_);
    std::string s = "select value from network where key='" + hex_key + "';";
    CppSQLite3Query q = db_.execQuery(s.c_str());
    while (!q.eof()) {
      results->push_back(base::DecodeFromHex(q.getStringField(0)));
      q.nextRow();
    }
  }
  catch(CppSQLite3Exception &e) {  // NOLINT
#ifdef DEBUG
    printf("Error(%i): %s\n", e.errorCode(),  e.errorMessage());
#endif
    return -2;
  }
  return kSuccess;
}

void LocalStoreManager::PollVaultInfo(base::callback_func_type cb) {
  boost::thread thr(boost::bind(&ExecCallbackVaultInfo, cb, &mutex_));
}

void LocalStoreManager::VaultContactInfo(base::callback_func_type cb) {
  boost::thread thr(boost::bind(&ExecuteSuccessCallback, cb, &mutex_));
}

void LocalStoreManager::SetLocalVaultOwned(const std::string &,
                                           const std::string &pub_key,
                                           const std::string &signed_pub_key,
                                           const boost::uint32_t &,
                                           const std::string &,
                                           const boost::uint64_t &,
                                           const SetLocalVaultOwnedFunctor &f) {
  crypto::Crypto co;
  co.set_hash_algorithm(crypto::SHA_512);
  std::string pmid_name = co.Hash(pub_key + signed_pub_key, "",
                          crypto::STRING_STRING, false);
  boost::thread thr(f, OWNED_SUCCESS, pmid_name);
}

void LocalStoreManager::LocalVaultOwned(const LocalVaultOwnedFunctor &functor) {
  boost::thread thr(functor, NOT_OWNED);
}

bool LocalStoreManager::NotDoneWithUploading() { return false; }

}  // namespace maidsafe
