// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/url_request_throttler_entry.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/rand_util.h"
#include "base/string_number_conversions.h"
#include "base/values.h"
#include "net/base/load_flags.h"
#include "net/base/net_log.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_throttler_header_interface.h"
#include "net/url_request/url_request_throttler_manager.h"

namespace net {

const int URLRequestThrottlerEntry::kDefaultSlidingWindowPeriodMs = 2000;
const int URLRequestThrottlerEntry::kDefaultMaxSendThreshold = 20;

// This set of back-off parameters will (at maximum values, i.e. without
// the reduction caused by jitter) add 0-41% (distributed uniformly
// in that range) to the "perceived downtime" of the remote server, once
// exponential back-off kicks in and is throttling requests for more than
// about a second at a time.  Once the maximum back-off is reached, the added
// perceived downtime decreases rapidly, percentage-wise.
//
// Another way to put it is that the maximum additional perceived downtime
// with these numbers is a couple of seconds shy of 15 minutes, and such
// a delay would not occur until the remote server has been actually
// unavailable at the end of each back-off period for a total of about
// 48 minutes.
//
// Ignoring the first couple of errors is just a conservative measure to
// avoid false positives.  It should help avoid back-off from kicking in e.g.
// on flaky connections.
const int URLRequestThrottlerEntry::kDefaultNumErrorsToIgnore = 2;
const int URLRequestThrottlerEntry::kDefaultInitialDelayMs = 700;
const double URLRequestThrottlerEntry::kDefaultMultiplyFactor = 1.4;
const double URLRequestThrottlerEntry::kDefaultJitterFactor = 0.4;
const int URLRequestThrottlerEntry::kDefaultMaximumBackoffMs = 15 * 60 * 1000;
const int URLRequestThrottlerEntry::kDefaultEntryLifetimeMs = 2 * 60 * 1000;
const char URLRequestThrottlerEntry::kExponentialThrottlingHeader[] =
    "X-Chrome-Exponential-Throttling";
const char URLRequestThrottlerEntry::kExponentialThrottlingDisableValue[] =
    "disable";

// Returns NetLog parameters when a request is rejected by throttling.
Value* NetLogRejectedRequestCallback(const std::string* url_id,
                                     int num_failures,
                                     int release_after_ms,
                                     NetLog::LogLevel /* log_level */) {
  DictionaryValue* dict = new DictionaryValue();
  dict->SetString("url", *url_id);
  dict->SetInteger("num_failures", num_failures);
  dict->SetInteger("release_after_ms", release_after_ms);
  return dict;
}

URLRequestThrottlerEntry::URLRequestThrottlerEntry(
    URLRequestThrottlerManager* manager,
    const std::string& url_id)
    : sliding_window_period_(
          base::TimeDelta::FromMilliseconds(kDefaultSlidingWindowPeriodMs)),
      max_send_threshold_(kDefaultMaxSendThreshold),
      is_backoff_disabled_(false),
      backoff_entry_(&backoff_policy_),
      manager_(manager),
      url_id_(url_id),
      net_log_(BoundNetLog::Make(
          manager->net_log(), NetLog::SOURCE_EXPONENTIAL_BACKOFF_THROTTLING)) {
  DCHECK(manager_);
  Initialize();
}

URLRequestThrottlerEntry::URLRequestThrottlerEntry(
    URLRequestThrottlerManager* manager,
    const std::string& url_id,
    int sliding_window_period_ms,
    int max_send_threshold,
    int initial_backoff_ms,
    double multiply_factor,
    double jitter_factor,
    int maximum_backoff_ms)
    : sliding_window_period_(
          base::TimeDelta::FromMilliseconds(sliding_window_period_ms)),
      max_send_threshold_(max_send_threshold),
      is_backoff_disabled_(false),
      backoff_entry_(&backoff_policy_),
      manager_(manager),
      url_id_(url_id) {
  DCHECK_GT(sliding_window_period_ms, 0);
  DCHECK_GT(max_send_threshold_, 0);
  DCHECK_GE(initial_backoff_ms, 0);
  DCHECK_GT(multiply_factor, 0);
  DCHECK_GE(jitter_factor, 0.0);
  DCHECK_LT(jitter_factor, 1.0);
  DCHECK_GE(maximum_backoff_ms, 0);
  DCHECK(manager_);

  Initialize();
  backoff_policy_.initial_delay_ms = initial_backoff_ms;
  backoff_policy_.multiply_factor = multiply_factor;
  backoff_policy_.jitter_factor = jitter_factor;
  backoff_policy_.maximum_backoff_ms = maximum_backoff_ms;
  backoff_policy_.entry_lifetime_ms = -1;
  backoff_policy_.num_errors_to_ignore = 0;
  backoff_policy_.always_use_initial_delay = false;
}

bool URLRequestThrottlerEntry::IsEntryOutdated() const {
  // This function is called by the URLRequestThrottlerManager to determine
  // whether entries should be discarded from its url_entries_ map.  We
  // want to ensure that it does not remove entries from the map while there
  // are clients (objects other than the manager) holding references to
  // the entry, otherwise separate clients could end up holding separate
  // entries for a request to the same URL, which is undesirable.  Therefore,
  // if an entry has more than one reference (the map will always hold one),
  // it should not be considered outdated.
  //
  // We considered whether to make URLRequestThrottlerEntry objects
  // non-refcounted, but since any means of knowing whether they are
  // currently in use by others than the manager would be more or less
  // equivalent to a refcount, we kept them refcounted.
  if (!HasOneRef())
    return false;

  // If there are send events in the sliding window period, we still need this
  // entry.
  if (!send_log_.empty() &&
      send_log_.back() + sliding_window_period_ > ImplGetTimeNow()) {
    return false;
  }

  return GetBackoffEntry()->CanDiscard();
}

void URLRequestThrottlerEntry::DisableBackoffThrottling() {
  is_backoff_disabled_ = true;
}

void URLRequestThrottlerEntry::DetachManager() {
  manager_ = NULL;
}

bool URLRequestThrottlerEntry::ShouldRejectRequest(
    const URLRequest& request) const {
  bool reject_request = false;
  if (!is_backoff_disabled_ && !ExplicitUserRequest(request.load_flags()) &&
      (!request.context()->network_delegate() ||
       request.context()->network_delegate()->CanThrottleRequest(request)) &&
      GetBackoffEntry()->ShouldRejectRequest()) {
    int num_failures = GetBackoffEntry()->failure_count();
    int release_after_ms =
        GetBackoffEntry()->GetTimeUntilRelease().InMilliseconds();

    net_log_.AddEvent(
        NetLog::TYPE_THROTTLING_REJECTED_REQUEST,
        base::Bind(&NetLogRejectedRequestCallback,
                   &url_id_, num_failures, release_after_ms));

    reject_request = true;
  }

  int reject_count = reject_request ? 1 : 0;
  UMA_HISTOGRAM_ENUMERATION(
      "Throttling.RequestThrottled", reject_count, 2);

  return reject_request;
}

int64 URLRequestThrottlerEntry::ReserveSendingTimeForNextRequest(
    const base::TimeTicks& earliest_time) {
  base::TimeTicks now = ImplGetTimeNow();

  // If a lot of requests were successfully made recently,
  // sliding_window_release_time_ may be greater than
  // exponential_backoff_release_time_.
  base::TimeTicks recommended_sending_time =
      std::max(std::max(now, earliest_time),
               std::max(GetBackoffEntry()->GetReleaseTime(),
                        sliding_window_release_time_));

  DCHECK(send_log_.empty() ||
         recommended_sending_time >= send_log_.back());
  // Log the new send event.
  send_log_.push(recommended_sending_time);

  sliding_window_release_time_ = recommended_sending_time;

  // Drop the out-of-date events in the event list.
  // We don't need to worry that the queue may become empty during this
  // operation, since the last element is sliding_window_release_time_.
  while ((send_log_.front() + sliding_window_period_ <=
          sliding_window_release_time_) ||
         send_log_.size() > static_cast<unsigned>(max_send_threshold_)) {
    send_log_.pop();
  }

  // Check if there are too many send events in recent time.
  if (send_log_.size() == static_cast<unsigned>(max_send_threshold_))
    sliding_window_release_time_ = send_log_.front() + sliding_window_period_;

  return (recommended_sending_time - now).InMillisecondsRoundedUp();
}

base::TimeTicks
    URLRequestThrottlerEntry::GetExponentialBackoffReleaseTime() const {
  // If a site opts out, it's likely because they have problems that trigger
  // the back-off mechanism when it shouldn't be triggered, in which case
  // returning the calculated back-off release time would probably be the
  // wrong thing to do (i.e. it would likely be too long).  Therefore, we
  // return "now" so that retries are not delayed.
  if (is_backoff_disabled_)
    return ImplGetTimeNow();

  return GetBackoffEntry()->GetReleaseTime();
}

void URLRequestThrottlerEntry::UpdateWithResponse(
    const std::string& host,
    const URLRequestThrottlerHeaderInterface* response) {
  if (IsConsideredError(response->GetResponseCode())) {
    GetBackoffEntry()->InformOfRequest(false);
  } else {
    GetBackoffEntry()->InformOfRequest(true);

    std::string throttling_header = response->GetNormalizedValue(
        kExponentialThrottlingHeader);
    if (!throttling_header.empty())
      HandleThrottlingHeader(throttling_header, host);
  }
}

void URLRequestThrottlerEntry::ReceivedContentWasMalformed(int response_code) {
  // A malformed body can only occur when the request to fetch a resource
  // was successful.  Therefore, in such a situation, we will receive one
  // call to ReceivedContentWasMalformed() and one call to
  // UpdateWithResponse() with a response categorized as "good".  To end
  // up counting one failure, we need to count two failures here against
  // the one success in UpdateWithResponse().
  //
  // We do nothing for a response that is already being considered an error
  // based on its status code (otherwise we would count 3 errors instead of 1).
  if (!IsConsideredError(response_code)) {
    GetBackoffEntry()->InformOfRequest(false);
    GetBackoffEntry()->InformOfRequest(false);
  }
}

URLRequestThrottlerEntry::~URLRequestThrottlerEntry() {
}

void URLRequestThrottlerEntry::Initialize() {
  sliding_window_release_time_ = base::TimeTicks::Now();
  backoff_policy_.num_errors_to_ignore = kDefaultNumErrorsToIgnore;
  backoff_policy_.initial_delay_ms = kDefaultInitialDelayMs;
  backoff_policy_.multiply_factor = kDefaultMultiplyFactor;
  backoff_policy_.jitter_factor = kDefaultJitterFactor;
  backoff_policy_.maximum_backoff_ms = kDefaultMaximumBackoffMs;
  backoff_policy_.entry_lifetime_ms = kDefaultEntryLifetimeMs;
  backoff_policy_.always_use_initial_delay = false;
}

bool URLRequestThrottlerEntry::IsConsideredError(int response_code) {
  // We throttle only for the status codes most likely to indicate the server
  // is failing because it is too busy or otherwise are likely to be
  // because of DDoS.
  //
  // 500 is the generic error when no better message is suitable, and
  //     as such does not necessarily indicate a temporary state, but
  //     other status codes cover most of the permanent error states.
  // 503 is explicitly documented as a temporary state where the server
  //     is either overloaded or down for maintenance.
  // 509 is the (non-standard but widely implemented) Bandwidth Limit Exceeded
  //     status code, which might indicate DDoS.
  //
  // We do not back off on 502 or 504, which are reported by gateways
  // (proxies) on timeouts or failures, because in many cases these requests
  // have not made it to the destination server and so we do not actually
  // know that it is down or busy.  One degenerate case could be a proxy on
  // localhost, where you are not actually connected to the network.
  return (response_code == 500 ||
          response_code == 503 ||
          response_code == 509);
}

base::TimeTicks URLRequestThrottlerEntry::ImplGetTimeNow() const {
  return base::TimeTicks::Now();
}

void URLRequestThrottlerEntry::HandleThrottlingHeader(
    const std::string& header_value,
    const std::string& host) {
  if (header_value == kExponentialThrottlingDisableValue) {
    DisableBackoffThrottling();
    if (manager_)
      manager_->AddToOptOutList(host);
  }
}

const BackoffEntry* URLRequestThrottlerEntry::GetBackoffEntry() const {
  return &backoff_entry_;
}

BackoffEntry* URLRequestThrottlerEntry::GetBackoffEntry() {
  return &backoff_entry_;
}

// static
bool URLRequestThrottlerEntry::ExplicitUserRequest(const int load_flags) {
  return (load_flags & LOAD_MAYBE_USER_GESTURE) != 0;
}

}  // namespace net
