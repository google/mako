// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CXX_INTERNAL_CLOCK_MOCK_H_
#define CXX_INTERNAL_CLOCK_MOCK_H_

#include "gmock/gmock.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "cxx/internal/clock.h"

namespace mako {
namespace internal {

class ClockMock : public helpers::Clock {
 public:
  ClockMock() : now_(absl::UnixEpoch()) {
    ON_CALL(*this, TimeNow())
        .WillByDefault(testing::Invoke(this, &ClockMock::current_time));
    ON_CALL(*this, Sleep(testing::A<absl::Duration>()))
        .WillByDefault(testing::Invoke(this, &ClockMock::SleepTime));
    ON_CALL(*this, SleepUntil(testing::A<absl::Time>()))
        .WillByDefault(testing::Invoke(this, &ClockMock::SetTime));
  }

  MOCK_METHOD(absl::Time, TimeNow, (), (override));
  MOCK_METHOD(void, Sleep, (absl::Duration d), (override));
  MOCK_METHOD(void, SleepUntil, (absl::Time wakeup_time), (override));

  absl::Time current_time() const { return now_; }

  // Set the mock time to 'timestamp'.
  void SetTime(absl::Time timestamp) { now_ = timestamp; }

  // Advance the mock time and return immediately.
  void SleepTime(absl::Duration d) { now_ += d; }

 private:
  absl::Time now_;  // The mock time.
};

}  // namespace internal
}  // namespace mako

#endif  // CXX_INTERNAL_CLOCK_MOCK_H_
