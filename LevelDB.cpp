/*
    Copyright (C) 2019-Present SKALE Labs

    This file is part of sgxwallet.

    sgxwallet is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    sgxwallet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with sgxwallet.  If not, see <https://www.gnu.org/licenses/>.

    @file LevelDB.cpp
    @author Stan Kladko
    @date 2019
*/


#include <stdexcept>
#include <memory>
#include <string>
#include <iostream>


#include "leveldb/db.h"

#include "sgxwallet_common.h"
#include "RPCException.h"
#include "LevelDB.h"

#include "ServerInit.h"

using namespace leveldb;


static WriteOptions writeOptions;
static ReadOptions readOptions;


LevelDB* levelDb = nullptr;
LevelDB* csrDb = nullptr;
LevelDB* csrStatusDb = nullptr;


std::shared_ptr<std::string> LevelDB::readString(const std::string &_key) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto result = std::make_shared<std::string>();

    if (db == nullptr) {
        throw RPCException(NULL_DATABASE, "Null db");
    }

    auto status = db->Get(readOptions, _key, &*result);

//    if (result == nullptr) {
//      throw RPCException(KEY_SHARE_DOES_NOT_EXIST, "Data with this name does not exist");
//    }
    if (DEBUG_PRINT) {
      std::cerr << "key to read from db: " << _key << std::endl;
    }

    throwExceptionOnError(status);

    if (status.IsNotFound())
        return nullptr;

    return result;
}

void LevelDB::writeString(const std::string &_key, const std::string &_value) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto status = db->Put(writeOptions, Slice(_key), Slice(_value));

    throwExceptionOnError(status);

    if (DEBUG_PRINT) {
      std::cerr << "written key " << _key  << std::endl;
    }
}


void LevelDB::deleteDHDKGKey (const std::string &_key) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    std::string full_key = "DKG_DH_KEY_" + _key;

    auto status = db->Delete(writeOptions, Slice(_key));

    throwExceptionOnError(status);

    std::cerr << "key deleted " << full_key << std::endl;
}

void LevelDB::deleteOlegKey (const std::string &_key) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    std::string full_key = "key" + _key;

    auto status = db->Delete(writeOptions, Slice(_key));

    throwExceptionOnError(status);

    std::cerr << "key deleted " << full_key << std::endl;
}

void LevelDB::deleteTempNEK(const std::string &_key){

    std::lock_guard<std::recursive_mutex> lock(mutex);

    std::string prefix = _key.substr(0,8);
    if (prefix != "tmp_NEK:") {
      return;
    }

    auto status = db->Delete(writeOptions, Slice(_key));

    throwExceptionOnError(status);

    std::cerr << "key deleted " << _key << std::endl;
}

void LevelDB::deleteKey(const std::string &_key){

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto status = db->Delete(writeOptions, Slice(_key));

    throwExceptionOnError(status);

    if (DEBUG_PRINT) {
      std::cerr << "key deleted " << _key << std::endl;
    }
}



void LevelDB::writeByteArray(const char *_key, size_t _keyLen, const char *value,
                             size_t _valueLen) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto status = db->Put(writeOptions, Slice(_key, _keyLen), Slice(value, _valueLen));

    throwExceptionOnError(status);
}


void LevelDB::writeByteArray(std::string &_key, const char *value,
                             size_t _valueLen) {

    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto status = db->Put(writeOptions, Slice(_key), Slice(value, _valueLen));

    throwExceptionOnError(status);
}

void LevelDB::throwExceptionOnError(Status _status) {

    if (_status.IsNotFound())
        return;

    if (!_status.ok()) {
        throw RPCException(COULD_NOT_ACCESS_DATABASE, ("Could not access database database:" + _status.ToString()).c_str());
    }

}

uint64_t LevelDB::visitKeys(LevelDB::KeyVisitor *_visitor, uint64_t _maxKeysToVisit) {

    uint64_t readCounter = 0;

    leveldb::Iterator *it = db->NewIterator(readOptions);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        _visitor->visitDBKey(it->key().data());
        readCounter++;
        if (readCounter >= _maxKeysToVisit) {
            break;
        }
    }

    delete it;

    return readCounter;
}

std::vector<std::string> LevelDB::writeKeysToVector1(uint64_t _maxKeysToVisit){
  uint64_t readCounter = 0;
  std::vector<std::string> keys;

  leveldb::Iterator *it = db->NewIterator(readOptions);
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    std::string cur_key(it->key().data(), it->key().size());
    keys.push_back(cur_key);
   // keys.push_back(it->key().data());
    readCounter++;
    if (readCounter >= _maxKeysToVisit) {
      break;
    }
  }

  delete it;

  return keys;
}

void LevelDB::writeDataUnique(const std::string & Name, const std::string &value) {

  auto key = Name;

  if (readString(Name) != nullptr) {
    std::cerr << "name " << Name << " already exists" << std::endl;
    throw RPCException(KEY_SHARE_ALREADY_EXISTS, "Data with this name already exists");
  }

  writeString(key, value);
  std::cerr << Name << " is written to db " << std::endl;
}


LevelDB::LevelDB(std::string &filename) {


    leveldb::Options options;
    options.create_if_missing = true;

    if (!leveldb::DB::Open(options, filename, (leveldb::DB **) &db).ok()) {
        throw std::runtime_error("Unable to open levelDB database");
    }

    if (db == nullptr) {
        throw std::runtime_error("Null levelDB object");
    }

}

LevelDB::~LevelDB() {
    if (db != nullptr)
        delete db;
}



