// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time.h"

#include <math.h>
#if defined(OS_WIN)
#include <float.h>
#endif

#include <limits>

#include "base/sys_string_conversions.h"
#include "base/third_party/nspr/prtime.h"

#include "base/logging.h"
#include "starboard/double.h"

namespace base {

namespace {
#if defined(OS_WIN)
inline bool isnan(double num) { return !!_isnan(num); }
#endif
}

// TimeDelta ------------------------------------------------------------------

// static
TimeDelta TimeDelta::Max() {
  return TimeDelta(std::numeric_limits<int64>::max());
}

int TimeDelta::InDays() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerDay);
}

int TimeDelta::InHours() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerHour);
}

int TimeDelta::InMinutes() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(delta_ / Time::kMicrosecondsPerMinute);
}

double TimeDelta::InSecondsF() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / Time::kMicrosecondsPerSecond;
}

int64 TimeDelta::InSeconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64>::max();
  }
  return delta_ / Time::kMicrosecondsPerSecond;
}

double TimeDelta::InMillisecondsF() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<double>::infinity();
  }
  return static_cast<double>(delta_) / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMilliseconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64>::max();
  }
  return delta_ / Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMillisecondsRoundedUp() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64>::max();
  }
  return (delta_ + Time::kMicrosecondsPerMillisecond - 1) /
      Time::kMicrosecondsPerMillisecond;
}

int64 TimeDelta::InMicroseconds() const {
  if (is_max()) {
    // Preserve max to prevent overflow.
    return std::numeric_limits<int64>::max();
  }
  return delta_;
}

// Time -----------------------------------------------------------------------

// static
Time Time::Max() {
  return Time(std::numeric_limits<int64>::max());
}

// static
Time Time::FromTimeT(time_t tt) {
  if (tt == 0)
    return Time();  // Preserve 0 so we can tell it doesn't exist.
  if (tt == std::numeric_limits<time_t>::max())
    return Max();
  return Time((tt * kMicrosecondsPerSecond) + kTimeTToMicrosecondsOffset);
}

time_t Time::ToTimeT() const {
  if (is_null())
    return 0;  // Preserve 0 so we can tell it doesn't exist.
  if (is_max()) {
    // Preserve max without offset to prevent overflow.
    return std::numeric_limits<time_t>::max();
  }
  if (std::numeric_limits<int64>::max() - kTimeTToMicrosecondsOffset <= us_) {
    DLOG(WARNING) << "Overflow when converting base::Time with internal " <<
                     "value " << us_ << " to time_t.";
    return std::numeric_limits<time_t>::max();
  }
  return (us_ - kTimeTToMicrosecondsOffset) / kMicrosecondsPerSecond;
}

// static
Time Time::FromDoubleT(double dt) {
  if (dt == 0 || SbDoubleIsNan(dt))
    return Time();  // Preserve 0 so we can tell it doesn't exist.
  if (dt == std::numeric_limits<double>::max())
    return Max();
  return Time(static_cast<int64>((dt *
                                  static_cast<double>(kMicrosecondsPerSecond)) +
                                 kTimeTToMicrosecondsOffset));
}

double Time::ToDoubleT() const {
  if (is_null())
    return 0;  // Preserve 0 so we can tell it doesn't exist.
  if (is_max()) {
    // Preserve max without offset to prevent overflow.
    return std::numeric_limits<double>::max();
  }
  return (static_cast<double>(us_ - kTimeTToMicrosecondsOffset) /
          static_cast<double>(kMicrosecondsPerSecond));
}

// static
Time Time::FromJsTime(double ms_since_epoch) {
  // The epoch is a valid time, so this constructor doesn't interpret
  // 0 as the null time.
  if (ms_since_epoch == std::numeric_limits<double>::max())
    return Max();
  return Time(static_cast<int64>(ms_since_epoch * kMicrosecondsPerMillisecond) +
              kTimeTToMicrosecondsOffset);
}

double Time::ToJsTime() const {
  if (is_null()) {
    // Preserve 0 so the invalid result doesn't depend on the platform.
    return 0;
  }
  if (is_max()) {
    // Preserve max without offset to prevent overflow.
    return std::numeric_limits<double>::max();
  }
  return (static_cast<double>(us_ - kTimeTToMicrosecondsOffset) /
          kMicrosecondsPerMillisecond);
}

// static
Time Time::UnixEpoch() {
  Time time;
  time.us_ = kTimeTToMicrosecondsOffset;
  return time;
}

Time Time::LocalMidnight() const {
  Exploded exploded;
  LocalExplode(&exploded);
  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;
  return FromLocalExploded(exploded);
}

// static
bool Time::FromStringInternal(const char* time_string,
                              bool is_local,
                              Time* parsed_time) {
  DCHECK((time_string != NULL) && (parsed_time != NULL));

  if (time_string[0] == '\0')
    return false;

  PRTime result_time = 0;
  PRStatus result = PR_ParseTimeString(time_string,
                                       is_local ? PR_FALSE : PR_TRUE,
                                       &result_time);
  if (PR_SUCCESS != result)
    return false;

  result_time += kTimeTToMicrosecondsOffset;
  *parsed_time = Time(result_time);
  return true;
}

// Time::Exploded -------------------------------------------------------------

inline bool is_in_range(int value, int lo, int hi) {
  return lo <= value && value <= hi;
}

bool Time::Exploded::HasValidValues() const {
  return is_in_range(month, 1, 12) &&
         is_in_range(day_of_week, 0, 6) &&
         is_in_range(day_of_month, 1, 31) &&
         is_in_range(hour, 0, 23) &&
         is_in_range(minute, 0, 59) &&
         is_in_range(second, 0, 60) &&
         is_in_range(millisecond, 0, 999);
}

}  // namespace base
