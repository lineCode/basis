// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "base/macros.h"
#include "basis/promise/abstract_promise.h"

namespace base {
namespace internal {

// An Executor that doesn't do anything.
class BASE_EXPORT NoOpPromiseExecutor {
 public:
  NoOpPromiseExecutor(bool can_resolve, bool can_reject);

  ~NoOpPromiseExecutor();

  static constexpr PromiseExecutor::PrerequisitePolicy kPrerequisitePolicy =
      PromiseExecutor::PrerequisitePolicy::kNever;

  static PassedPromise Create(Location from_here,
                              bool can_resolve,
                              bool can_reject,
                              RejectPolicy reject_policy);

  PromiseExecutor::PrerequisitePolicy GetPrerequisitePolicy() const;
  bool IsCancelled() const;

#if DCHECK_IS_ON()
  PromiseExecutor::ArgumentPassingType ResolveArgumentPassingType() const;
  PromiseExecutor::ArgumentPassingType RejectArgumentPassingType() const;
  bool CanResolve() const;
  bool CanReject() const;
#endif
  void Execute(AbstractPromise* promise);

 private:
#if DCHECK_IS_ON()
  bool can_resolve_;
  bool can_reject_;
#endif
};

}  // namespace internal
}  // namespace base
