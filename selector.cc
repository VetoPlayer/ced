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
#include "selector.h"

static bool RuleMatches(const std::string& selector, const std::string& token) {
  return selector.length() <= token.length() &&
         std::mismatch(selector.begin(), selector.end(), token.begin()).first ==
             selector.end();
}

bool SelectorMatches(Selector selector, Tag token) {
  if (selector.Empty()) return true;
  if (token.Empty()) return false;
  if (RuleMatches(selector.Head(), token.Head())) {
    return SelectorMatches(selector.Tail(), token.Tail());
  } else {
    return SelectorMatches(selector, token.Tail());
  }
}
