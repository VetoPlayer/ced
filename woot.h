// Copyright 2017 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <assert.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "avl.h"
#include "crdt.h"

class String : public CRDT<String> {
 public:
  String() {
    avl_ =
        avl_.Add(Begin(), CharInfo{false, char(), End(), End(), End(), End()})
            .Add(End(),
                 CharInfo{false, char(), Begin(), Begin(), Begin(), Begin()});
    line_breaks_ = line_breaks_.Add(Begin(), LineBreak{End(), End()})
                       .Add(End(), LineBreak{Begin(), Begin()});
  }

  static ID Begin() { return begin_id_; }
  static ID End() { return end_id_; }

  // return <0 if a before b, >0 if a after b, ==0 if a==b
  int OrderIDs(ID a, ID b) const;

  bool Has(ID id) const { return avl_.Lookup(id) != nullptr; }

  static ID MakeRawInsert(CommandBuf* buf, Site* site, char c, ID after,
                          ID before) {
    return MakeCommand(buf, site->GenerateID(),
                       [c, after, before](String s, ID id) {
                         return s.IntegrateInsert(id, c, after, before);
                       });
  }

  static ID MakeRawInsert(CommandBuf* buf, Site* site, const std::string& s,
                          ID after, ID before) {
    // TODO(ctiller): make this an optimization
    for (auto c : s) {
      after = MakeRawInsert(buf, site, c, after, before);
    }
    return after;
  }

  template <class T>
  ID MakeInsert(CommandBuf* buf, Site* site, const T& c, ID after) const {
    return MakeRawInsert(buf, site, c, after, avl_.Lookup(after)->next);
  }

  void MakeRemove(CommandBuf* buf, ID chr) const {
    MakeCommand(buf, chr,
                [](String s, ID id) { return s.IntegrateRemove(id); });
  }

  void MakeRemove(CommandBuf* buf, ID beg, ID end) const;

  std::string Render() const;
  std::string Render(ID beg, ID end) const;

  bool SameIdentity(String s) const { return avl_.SameIdentity(s.avl_); }

 private:
  struct CharInfo {
    // tombstone if false
    bool visible;
    // glyph
    char chr;
    // next/prev in document
    ID next;
    ID prev;
    // before/after in insert order (according to creator)
    ID after;
    ID before;
  };

  struct LineBreak {
    ID prev;
    ID next;
  };

  String(AVL<ID, CharInfo> avl, AVL<ID, LineBreak> line_breaks)
      : avl_(avl), line_breaks_(line_breaks) {}

  String IntegrateRemove(ID id) const;
  String IntegrateInsert(ID id, char c, ID after, ID before) const;

  AVL<ID, CharInfo> avl_;
  AVL<ID, LineBreak> line_breaks_;
  static Site root_site_;
  static ID begin_id_;
  static ID end_id_;

 public:
  class AllIterator {
   public:
    AllIterator(const String& str, ID where)
        : str_(&str), pos_(where), cur_(str_->avl_.Lookup(pos_)) {}

    bool is_end() const { return pos_ == End(); }
    bool is_begin() const { return pos_ == Begin(); }

    ID id() const { return pos_; }
    char value() const { return cur_->chr; }
    bool is_visible() const { return cur_->visible; }

    void MoveNext() {
      pos_ = cur_->next;
      cur_ = str_->avl_.Lookup(pos_);
    }
    void MovePrev() {
      pos_ = cur_->prev;
      cur_ = str_->avl_.Lookup(pos_);
    }

   private:
    const String* str_;
    ID pos_;
    const CharInfo* cur_;
  };

  class Iterator {
   public:
    Iterator(const String& str, ID where) : it_(str, where) {
      while (!is_begin() && !it_.is_visible()) {
        it_.MovePrev();
      }
    }

    bool is_end() const { return it_.is_end(); }
    bool is_begin() const { return it_.is_begin(); }

    ID id() const { return it_.id(); }
    char value() const { return it_.value(); }

    void MoveNext() {
      if (!is_end()) it_.MoveNext();
      while (!is_end() && !it_.is_visible()) it_.MoveNext();
    }

    void MovePrev() {
      if (!is_begin()) it_.MovePrev();
      while (!is_begin() && !it_.is_visible()) it_.MovePrev();
    }

    Iterator Prev() {
      Iterator i(*this);
      i.MovePrev();
      return i;
    }

   private:
    AllIterator it_;
  };

  class LineIterator {
   public:
    LineIterator(const String& str, ID where) : str_(&str) {
      Iterator it(str, where);
      const LineBreak* lb = str.line_breaks_.Lookup(it.id());
      while (lb == nullptr) {
        it.MovePrev();
        lb = str.line_breaks_.Lookup(it.id());
      }
      id_ = it.id();
    }

    bool is_end() const { return id_ == End(); }
    bool is_begin() const { return id_ == Begin(); }

    void MovePrev() {
      if (id_ == Begin()) return;
      id_ = str_->line_breaks_.Lookup(id_)->prev;
    }

    void MoveNext() {
      if (id_ == End()) return;
      id_ = str_->line_breaks_.Lookup(id_)->next;
    }

    LineIterator Next() {
      LineIterator tmp(*this);
      tmp.MoveNext();
      return tmp;
    }

    Iterator AsIterator() { return Iterator(*str_, id_); }
    AllIterator AsAllIterator() { return AllIterator(*str_, id_); }

    ID id() const { return id_; }

   private:
    const String* str_;
    ID id_;
  };
};
