// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_VERSION_EDIT_H_
#define STORAGE_LEVELDB_DB_VERSION_EDIT_H_

#include <set>
#include <utility>
#include <vector>
#include <time.h>
#include <chrono>

#include "db/dbformat.h"

namespace leveldb {

class VersionSet;

struct FileMetaData {
  FileMetaData() : refs(0), allowed_seeks(1 << 30), file_size(0),crt_cpt_id(0),del_cpt_id(0) {}

  int refs;
  int allowed_seeks;  // Seeks allowed until compaction
  //uint64_t level;
  uint64_t number;
  uint64_t file_size;    // File size in bytes
  InternalKey smallest;  // Smallest internal key served by table
  InternalKey largest;   // Largest internal key served by table
  uint64_t create_time; //timestamp from 01-01-1970 (seconds)
  uint64_t delete_time;
  uint64_t real_lifetime;
  uint64_t avg_est;
  uint64_t cal_est;
  uint64_t est_time;
  uint64_t crt_cpt_id;
  uint64_t del_cpt_id;
  bool is_sizecompaction;
  bool is_passive;
  bool is_deletedtag;
  //uint64_t active_est_lifetime; //
  //uint64_t passive_est_lifetime;
};

class VersionEdit {
 public:
  VersionEdit() { Clear(); }
  ~VersionEdit() = default;

  void Clear();

  void SetComparatorName(const Slice& name) {
    has_comparator_ = true;
    comparator_ = name.ToString();
  }
  void SetLogNumber(uint64_t num) {
    has_log_number_ = true;
    log_number_ = num;
  }
  void SetPrevLogNumber(uint64_t num) {
    has_prev_log_number_ = true;
    prev_log_number_ = num;
  }
  void SetNextFile(uint64_t num) {
    has_next_file_number_ = true;
    next_file_number_ = num;
  }
  void SetLastSequence(SequenceNumber seq) {
    has_last_sequence_ = true;
    last_sequence_ = seq;
  }
  void SetCompactPointer(int level, const InternalKey& key) {
    compact_pointers_.push_back(std::make_pair(level, key));
  }

  // Add the specified file at the specified number.
  // REQUIRES: This version has not been saved (see VersionSet::SaveTo)
  // REQUIRES: "smallest" and "largest" are smallest and largest keys in file
  void AddFile(int level, uint64_t file, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest) {
    FileMetaData f;
    f.number = file;
    f.file_size = file_size;
    f.smallest = smallest;
    f.largest = largest;
    new_files_.push_back(std::make_pair(level, f));
  }

  // add files to level0
    void AddFile0(int level, uint64_t file, uint64_t file_size,
                  const InternalKey& smallest, const InternalKey& largest, uint64_t est_time) {
        FileMetaData f;
        f.number = file;
        f.file_size = file_size;
        f.smallest = smallest;
        f.largest = largest;
        f.cal_est = 0;
        f.avg_est = est_time;
        f.est_time = est_time;
        f.crt_cpt_id = 0;
        f.del_cpt_id = 0;
        new_files_.push_back(std::make_pair(level, f));
    }

  void AddFile2(int level, uint64_t file, uint64_t file_size,
          const InternalKey& smallest, const InternalKey& largest, uint64_t compact_id, uint64_t cal_est, uint64_t avg_est) {
      FileMetaData f;
      f.number = file;
      f.file_size = file_size;
      f.smallest = smallest;
      f.largest = largest;
      f.crt_cpt_id = compact_id;
      f.del_cpt_id = 0;
      //f.est_lifetime = estimate_lifetime;
      f.cal_est = cal_est;
      f.avg_est = avg_est;
      //TODO() calculate the estimation lifetime based on cal_est & avg_est
      // f.est_time =
      if(cal_est == 0){
          f.est_time = avg_est;
      }else if(avg_est == 0){
          f.est_time = cal_est;
      }else{
          if((cal_est > avg_est) && (cal_est-avg_est)/avg_est >= 1){
              f.est_time = avg_est;
          }else if((avg_est > cal_est) && (avg_est-cal_est)/cal_est >= 1){
              f.est_time = cal_est;
          }else{
              f.est_time = (cal_est+avg_est)/2;
          }
      }
      new_files_.push_back(std::make_pair(level, f));
  }

 /* void AddFile3(int level, uint64_t file, uint64_t file_size,
                const InternalKey& smallest, const InternalKey& largest, uint64_t active_lifetime, uint64_t passive_lifetime){
      FileMetaData f;
      f.number = file;
      f.file_size = file_size;
      f.smallest = smallest;
      f.largest = largest;
      f.active_est_lifetime = active_lifetime;
      f.passive_est_lifetime = passive_lifetime;
      new_files_.push_back(std::make_pair(level, f));
  }
*/
  // Delete the specified "file" from the specified "level".
  void RemoveFile(int level, uint64_t file) {
    deleted_files_.insert(std::make_pair(level, file));
  }

  void RemoveFile2(int level, uint64_t file, bool is_sizecompaction, bool is_passive, uint64_t compact_id){
      FileMetaData f;
      //f.level = level;
      f.number = file;
      f.is_sizecompaction = is_sizecompaction;
      f.is_passive = is_passive;
      f.del_cpt_id = compact_id;
      deleted_files_2.push_back(std::make_pair(level,f));
  }

  void RemoveFile3(int level, FileMetaData *f){
      deleted_files_2.push_back(std::make_pair(level,*f));
  }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(const Slice& src);

  std::string DebugString() const;

  bool IsTrivialMove() {return is_trivial_move_;}

  void SetTrivalMove() { is_trivial_move_ = true;}

 private:
  friend class VersionSet;

  typedef std::set<std::pair<int, uint64_t>> DeletedFileSet;

  std::string comparator_;
  uint64_t log_number_;
  uint64_t prev_log_number_;
  uint64_t next_file_number_;
  SequenceNumber last_sequence_;
  bool has_comparator_;
  bool has_log_number_;
  bool has_prev_log_number_;
  bool has_next_file_number_;
  bool has_last_sequence_;
  bool is_trivial_move_ = false;

  std::vector<std::pair<int, InternalKey>> compact_pointers_;
  DeletedFileSet deleted_files_;
  std::vector<std::pair<int, FileMetaData>> deleted_files_2;
  std::vector<std::pair<int, FileMetaData>> new_files_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_EDIT_H_
