// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/post_task_and_reply_impl.h"

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "base/location.h"
#include "base/message_loop_proxy.h"

namespace base {

namespace {

// This relay class remembers the MessageLoop that it was created on, and
// ensures that both the |task| and |reply| Closures are deleted on this same
// thread. Also, |task| is guaranteed to be deleted before |reply| is run or
// deleted.
//
// If this is not possible because the originating MessageLoop is no longer
// available, the the |task| and |reply| Closures are leaked.  Leaking is
// considered preferable to having a thread-safetey violations caused by
// invoking the Closure destructor on the wrong thread.
class PostTaskAndReplyRelay {
 public:
  PostTaskAndReplyRelay(const tracked_objects::Location& from_here,
                        const Closure& task, const Closure& reply)
      : from_here_(from_here),
        origin_loop_(MessageLoopProxy::current()) {
    task_ = task;
    reply_ = reply;
  }

  ~PostTaskAndReplyRelay() {
    DCHECK(origin_loop_->BelongsToCurrentThread());
    task_.Reset();
    reply_.Reset();
  }

  void Run() {
    task_.Run();
    origin_loop_->PostTask(
        from_here_,
        Bind(&PostTaskAndReplyRelay::RunReplyAndSelfDestruct,
             base::Unretained(this)));
  }

 private:
  void RunReplyAndSelfDestruct() {
    DCHECK(origin_loop_->BelongsToCurrentThread());

    // Force |task_| to be released before |reply_| is to ensure that no one
    // accidentally depends on |task_| keeping one of its arguments alive while
    // |reply_| is executing.
    task_.Reset();

    reply_.Run();

    // Cue mission impossible theme.
    delete this;
  }

  tracked_objects::Location from_here_;
  scoped_refptr<MessageLoopProxy> origin_loop_;
  Closure reply_;
  Closure task_;
};

}  // namespace

namespace internal {

bool PostTaskAndReplyImpl::PostTaskAndReply(
    const tracked_objects::Location& from_here,
    const Closure& task,
    const Closure& reply) {
  // As mentioned in a comment above, this code is prepared to leak in the case
  // that the original calling message loop goes away while the task is being
  // processed.  Therefore, explicitly recognize this possibility by allowing
  // memory leaks here.
  ANNOTATE_SCOPED_MEMORY_LEAK;
  PostTaskAndReplyRelay* relay =
      new PostTaskAndReplyRelay(from_here, task, reply);
  if (!PostTask(from_here, Bind(&PostTaskAndReplyRelay::Run,
                                Unretained(relay)))) {
    delete relay;
    return false;
  }

  return true;
}

}  // namespace internal

}  // namespace base
