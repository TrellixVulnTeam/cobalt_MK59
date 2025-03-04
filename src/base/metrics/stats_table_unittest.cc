// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/stats_counters.h"
#include "base/metrics/stats_table.h"
#include "base/shared_memory.h"
#include "base/stringprintf.h"
#include "base/string_piece.h"
#include "base/test/multiprocess_test.h"
#include "base/threading/platform_thread.h"
#include "base/threading/simple_thread.h"
#include "base/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace base {

class StatsTableTest : public MultiProcessTest {
 public:
  void DeleteShmem(const std::string& name) {
    SharedMemory mem;
    mem.Delete(name);
  }
};

// Open a StatsTable and verify that we can write to each of the
// locations in the table.
TEST_F(StatsTableTest, VerifySlots) {
  const std::string kTableName = "VerifySlotsStatTable";
  const int kMaxThreads = 1;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);

  // Register a single thread.
  std::string thread_name = "mainThread";
  int slot_id = table.RegisterThread(thread_name);
  EXPECT_NE(slot_id, 0);

  // Fill up the table with counters.
  std::string counter_base_name = "counter";
  for (int index = 0; index < kMaxCounter; index++) {
    std::string counter_name = counter_base_name;
    base::StringAppendF(&counter_name, "counter.ctr%d", index);
    int counter_id = table.FindCounter(counter_name);
    EXPECT_GT(counter_id, 0);
  }

  // Try to allocate an additional thread.  Verify it fails.
  slot_id = table.RegisterThread("too many threads");
  EXPECT_EQ(slot_id, 0);

  // Try to allocate an additional counter.  Verify it fails.
  int counter_id = table.FindCounter(counter_base_name);
  EXPECT_EQ(counter_id, 0);

  DeleteShmem(kTableName);
}

// CounterZero will continually be set to 0.
const std::string kCounterZero = "CounterZero";
// Counter1313 will continually be set to 1313.
const std::string kCounter1313 = "Counter1313";
// CounterIncrement will be incremented each time.
const std::string kCounterIncrement = "CounterIncrement";
// CounterDecrement will be decremented each time.
const std::string kCounterDecrement = "CounterDecrement";
// CounterMixed will be incremented by odd numbered threads and
// decremented by even threads.
const std::string kCounterMixed = "CounterMixed";
// The number of thread loops that we will do.
const int kThreadLoops = 100;

class StatsTableThread : public SimpleThread {
 public:
  StatsTableThread(std::string name, int id)
      : SimpleThread(name),
        id_(id) {}

  virtual void Run() override;

 private:
  int id_;
};

void StatsTableThread::Run() {
  // Each thread will open the shared memory and set counters
  // concurrently in a loop.  We'll use some pauses to
  // mixup the thread scheduling.

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  for (int index = 0; index < kThreadLoops; index++) {
    StatsCounter mixed_counter(kCounterMixed);  // create this one in the loop
    zero_counter.Set(0);
    lucky13_counter.Set(1313);
    increment_counter.Increment();
    decrement_counter.Decrement();
    if (id_ % 2)
      mixed_counter.Decrement();
    else
      mixed_counter.Increment();
    PlatformThread::Sleep(TimeDelta::FromMilliseconds(index % 10));
  }
}

// Create a few threads and have them poke on their counters.
// See http://crbug.com/10611 for more information.
#if defined(OS_MACOSX)
#define MAYBE_MultipleThreads DISABLED_MultipleThreads
#else
#define MAYBE_MultipleThreads MultipleThreads
#endif
TEST_F(StatsTableTest, MAYBE_MultipleThreads) {
  // Create a stats table.
  const std::string kTableName = "MultipleThreadStatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  EXPECT_EQ(0, table.CountThreadsRegistered());

  // Spin up a set of threads to go bang on the various counters.
  // After we join the threads, we'll make sure the counters
  // contain the values we expected.
  StatsTableThread* threads[kMaxThreads];

  // Spawn the threads.
  for (int index = 0; index < kMaxThreads; index++) {
    threads[index] = new StatsTableThread("MultipleThreadsTest", index);
    threads[index]->Start();
  }

  // Wait for the threads to finish.
  for (int index = 0; index < kMaxThreads; index++) {
    threads[index]->Join();
    delete threads[index];
  }

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  StatsCounter mixed_counter(kCounterMixed);

  // Verify the various counters are correct.
  std::string name;
  name = "c:" + kCounterZero;
  EXPECT_EQ(0, table.GetCounterValue(name));
  name = "c:" + kCounter1313;
  EXPECT_EQ(1313 * kMaxThreads,
      table.GetCounterValue(name));
  name = "c:" + kCounterIncrement;
  EXPECT_EQ(kMaxThreads * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterDecrement;
  EXPECT_EQ(-kMaxThreads * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterMixed;
  EXPECT_EQ((kMaxThreads % 2) * kThreadLoops,
      table.GetCounterValue(name));
  EXPECT_EQ(0, table.CountThreadsRegistered());

  DeleteShmem(kTableName);
}

const std::string kMPTableName = "MultipleProcessStatTable";

MULTIPROCESS_TEST_MAIN(StatsTableMultipleProcessMain) {
  // Each process will open the shared memory and set counters
  // concurrently in a loop.  We'll use some pauses to
  // mixup the scheduling.

  StatsTable table(kMPTableName, 0, 0);
  StatsTable::set_current(&table);
  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);
  for (int index = 0; index < kThreadLoops; index++) {
    zero_counter.Set(0);
    lucky13_counter.Set(1313);
    increment_counter.Increment();
    decrement_counter.Decrement();
    PlatformThread::Sleep(TimeDelta::FromMilliseconds(index % 10));
  }
  return 0;
}

// Create a few processes and have them poke on their counters.
// This test is slow and flaky http://crbug.com/10611
TEST_F(StatsTableTest, DISABLED_MultipleProcesses) {
  // Create a stats table.
  const int kMaxProcs = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kMPTableName);
  StatsTable table(kMPTableName, kMaxProcs, kMaxCounter);
  StatsTable::set_current(&table);
  EXPECT_EQ(0, table.CountThreadsRegistered());

  // Spin up a set of processes to go bang on the various counters.
  // After we join the processes, we'll make sure the counters
  // contain the values we expected.
  ProcessHandle procs[kMaxProcs];

  // Spawn the processes.
  for (int16 index = 0; index < kMaxProcs; index++) {
    procs[index] = this->SpawnChild("StatsTableMultipleProcessMain", false);
    EXPECT_NE(kNullProcessHandle, procs[index]);
  }

  // Wait for the processes to finish.
  for (int index = 0; index < kMaxProcs; index++) {
    EXPECT_TRUE(WaitForSingleProcess(
        procs[index], base::TimeDelta::FromMinutes(1)));
    CloseProcessHandle(procs[index]);
  }

  StatsCounter zero_counter(kCounterZero);
  StatsCounter lucky13_counter(kCounter1313);
  StatsCounter increment_counter(kCounterIncrement);
  StatsCounter decrement_counter(kCounterDecrement);

  // Verify the various counters are correct.
  std::string name;
  name = "c:" + kCounterZero;
  EXPECT_EQ(0, table.GetCounterValue(name));
  name = "c:" + kCounter1313;
  EXPECT_EQ(1313 * kMaxProcs,
      table.GetCounterValue(name));
  name = "c:" + kCounterIncrement;
  EXPECT_EQ(kMaxProcs * kThreadLoops,
      table.GetCounterValue(name));
  name = "c:" + kCounterDecrement;
  EXPECT_EQ(-kMaxProcs * kThreadLoops,
      table.GetCounterValue(name));
  EXPECT_EQ(0, table.CountThreadsRegistered());

  DeleteShmem(kMPTableName);
}

class MockStatsCounter : public StatsCounter {
 public:
  explicit MockStatsCounter(const std::string& name)
      : StatsCounter(name) {}
  int* Pointer() { return GetPtr(); }
};

// Test some basic StatsCounter operations
TEST_F(StatsTableTest, StatsCounter) {
  // Create a stats table.
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  MockStatsCounter foo("foo");

  // Test initial state.
  EXPECT_TRUE(foo.Enabled());
  ASSERT_NE(foo.Pointer(), static_cast<int*>(0));
  EXPECT_EQ(0, *(foo.Pointer()));
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));

  // Test Increment.
  while (*(foo.Pointer()) < 123) foo.Increment();
  EXPECT_EQ(123, table.GetCounterValue("c:foo"));
  foo.Add(0);
  EXPECT_EQ(123, table.GetCounterValue("c:foo"));
  foo.Add(-1);
  EXPECT_EQ(122, table.GetCounterValue("c:foo"));

  // Test Set.
  foo.Set(0);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));
  foo.Set(100);
  EXPECT_EQ(100, table.GetCounterValue("c:foo"));
  foo.Set(-1);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Set(0);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));

  // Test Decrement.
  foo.Subtract(1);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Subtract(0);
  EXPECT_EQ(-1, table.GetCounterValue("c:foo"));
  foo.Subtract(-1);
  EXPECT_EQ(0, table.GetCounterValue("c:foo"));

  DeleteShmem(kTableName);
}

class MockStatsCounterTimer : public StatsCounterTimer {
 public:
  explicit MockStatsCounterTimer(const std::string& name)
      : StatsCounterTimer(name) {}

  TimeTicks start_time() { return start_time_; }
  TimeTicks stop_time() { return stop_time_; }
};

// Test some basic StatsCounterTimer operations
TEST_F(StatsTableTest, StatsCounterTimer) {
  // Create a stats table.
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  MockStatsCounterTimer bar("bar");

  // Test initial state.
  EXPECT_FALSE(bar.Running());
  EXPECT_TRUE(bar.start_time().is_null());
  EXPECT_TRUE(bar.stop_time().is_null());

  const TimeDelta kDuration = TimeDelta::FromMilliseconds(100);

  // Do some timing.
  bar.Start();
  PlatformThread::Sleep(kDuration);
  bar.Stop();
  EXPECT_GT(table.GetCounterValue("t:bar"), 0);
  EXPECT_LE(kDuration.InMilliseconds(), table.GetCounterValue("t:bar"));

  // Verify that timing again is additive.
  bar.Start();
  PlatformThread::Sleep(kDuration);
  bar.Stop();
  EXPECT_GT(table.GetCounterValue("t:bar"), 0);
  EXPECT_LE(kDuration.InMilliseconds() * 2, table.GetCounterValue("t:bar"));
  DeleteShmem(kTableName);
}

// Test some basic StatsRate operations
TEST_F(StatsTableTest, StatsRate) {
  // Create a stats table.
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  StatsRate baz("baz");

  // Test initial state.
  EXPECT_FALSE(baz.Running());
  EXPECT_EQ(0, table.GetCounterValue("c:baz"));
  EXPECT_EQ(0, table.GetCounterValue("t:baz"));

  const TimeDelta kDuration = TimeDelta::FromMilliseconds(100);

  // Do some timing.
  baz.Start();
  PlatformThread::Sleep(kDuration);
  baz.Stop();
  EXPECT_EQ(1, table.GetCounterValue("c:baz"));
  EXPECT_LE(kDuration.InMilliseconds(), table.GetCounterValue("t:baz"));

  // Verify that timing again is additive.
  baz.Start();
  PlatformThread::Sleep(kDuration);
  baz.Stop();
  EXPECT_EQ(2, table.GetCounterValue("c:baz"));
  EXPECT_LE(kDuration.InMilliseconds() * 2, table.GetCounterValue("t:baz"));
  DeleteShmem(kTableName);
}

// Test some basic StatsScope operations
TEST_F(StatsTableTest, StatsScope) {
  // Create a stats table.
  const std::string kTableName = "StatTable";
  const int kMaxThreads = 20;
  const int kMaxCounter = 5;
  DeleteShmem(kTableName);
  StatsTable table(kTableName, kMaxThreads, kMaxCounter);
  StatsTable::set_current(&table);

  StatsCounterTimer foo("foo");
  StatsRate bar("bar");

  // Test initial state.
  EXPECT_EQ(0, table.GetCounterValue("t:foo"));
  EXPECT_EQ(0, table.GetCounterValue("t:bar"));
  EXPECT_EQ(0, table.GetCounterValue("c:bar"));

  const TimeDelta kDuration = TimeDelta::FromMilliseconds(100);

  // Try a scope.
  {
    StatsScope<StatsCounterTimer> timer(foo);
    StatsScope<StatsRate> timer2(bar);
    PlatformThread::Sleep(kDuration);
  }
  EXPECT_LE(kDuration.InMilliseconds(), table.GetCounterValue("t:foo"));
  EXPECT_LE(kDuration.InMilliseconds(), table.GetCounterValue("t:bar"));
  EXPECT_EQ(1, table.GetCounterValue("c:bar"));

  // Try a second scope.
  {
    StatsScope<StatsCounterTimer> timer(foo);
    StatsScope<StatsRate> timer2(bar);
    PlatformThread::Sleep(kDuration);
  }
  EXPECT_LE(kDuration.InMilliseconds() * 2, table.GetCounterValue("t:foo"));
  EXPECT_LE(kDuration.InMilliseconds() * 2, table.GetCounterValue("t:bar"));
  EXPECT_EQ(2, table.GetCounterValue("c:bar"));

  DeleteShmem(kTableName);
}

}  // namespace base
