/*******************************************************************************
 *  Copyright 2011 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the license   *
 *  file LICENSE.TXT found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ***************************************************************************//**
 * @file  data_map.h
 * @brief Provides data structures for the DataMap.
 * @date  2008-09-09
 */

#ifndef MAIDSAFE_ENCRYPT_DATA_MAP_H_
#define MAIDSAFE_ENCRYPT_DATA_MAP_H_

#include <cstdint>
#include <string>
#include <vector>

#include "boost/serialization/string.hpp"
#include "boost/serialization/vector.hpp"
#include "maidsafe/encrypt/version.h"

#if MAIDSAFE_ENCRYPT_VERSION != 906
#  error This API is not compatible with the installed library.\
    Please update the maidsafe-encrypt library.
#endif

namespace maidsafe {

namespace encrypt {

/// Available types of self-encryption
enum SelfEncryptionTypes {
  // Type of hashing used (first 4 bits)
  kHashingSha1 = 1,
  kHashingSha512 = 2,
  kHashingTiger = 3,
  kHashingMask = 0xF,
  // Type of compression used (second 4 bits)
  kCompressionNone = 1 << 4,
  kCompressionGzip = 2 << 4,
  kCompressionMask = 0xF << 4,
  // Type of obfuscation used (third 4 bits)
  kObfuscationNone = 1 << 8,
  kObfuscationRepeated = 2 << 8,
  kObfuscationMask = 0xF << 8,
  // Type of cryptography used (fourth 4 bits)
  kCryptoNone = 1 << 12,
  kCryptoAes256 = 2 << 12,
  kCryptoMask = 0xF << 12
};

/// Holds information about a chunk
struct ChunkDetails {
  ChunkDetails() : hash(), size(0), pre_hash(), pre_size(0) {}
  std::string hash;        ///< Hash of processed chunk
  uint32_t size;      ///< Size of processed chunk
  std::string pre_hash;    ///< Hash of unprocessed source data
  uint32_t pre_size;  ///< Size of unprocessed source data
};

/// Holds information about the building blocks of a data item
struct DataMap {
  DataMap() : self_encryption_type(0), chunks(), size(0), content() {}
  uint32_t self_encryption_type;  ///< Type of SE used for chunks
  std::vector<ChunkDetails> chunks;  ///< Information about the chunks
  uint64_t size;      ///< Size of data item
  std::string content;     ///< Whole data item or last chunk, if small enough
};

}  // namespace encrypt

}  // namespace maidsafe

namespace boost {

namespace serialization {

template<class Archive>
void serialize(Archive &archive,  // NOLINT
               maidsafe::encrypt::ChunkDetails &chunk_details,
               const unsigned int /* version */) {
  archive & chunk_details.hash;
  archive & chunk_details.size;
  archive & chunk_details.pre_hash;
  archive & chunk_details.pre_size;
}

template<class Archive>
void serialize(Archive &archive,  // NOLINT
               maidsafe::encrypt::DataMap &data_map,
               const unsigned int /* version */) {
  archive & data_map.self_encryption_type;
  archive & data_map.chunks;
  archive & data_map.size;
  archive & data_map.content;
}

}  // namespace serialization

}  // namespace boost

#endif  // MAIDSAFE_ENCRYPT_DATA_MAP_H_
