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

#include "buffer.h"

class IOCollaborator final : public AsyncCollaborator {
 public:
  IOCollaborator(const Buffer* buffer);
  void Push(const EditNotification& notification) override;
  EditResponse Pull() override;

 private:
  absl::Mutex mu_;
  const std::string filename_;
  int attributes_;
  int fd_;
  ID last_char_id_ GUARDED_BY(mu_);
  String last_saved_ GUARDED_BY(mu_);
};
