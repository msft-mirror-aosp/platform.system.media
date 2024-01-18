/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <audio_utils/mutex.h>

#include <android-base/logging.h>
#include <benchmark/benchmark.h>
#include <shared_mutex>
#include <thread>
#include <utils/RWLock.h>
#include <utils/Timers.h>

/*
On Pixel 7 U arm64-v8a

Note: to bump up the scheduler clock frequency, one can use the toybox uclampset:
$ adb shell uclampset -m 1024 /data/benchmarktest64/audio_mutex_benchmark/audio_mutex_benchmark

For simplicity these tests use the regular invocation:
$ atest audio_mutex_benchmark

Benchmark                                                     Time        CPU        Iteration
audio_mutex_benchmark:
  #BM_atomic_add_equals<int32_t>                               6.5014863185278395 ns      6.471886635411762 ns     108155820
  #BM_atomic_add_to_seq_cst<int16_t>                             6.61234085779295 ns      6.581580170972295 ns     106366123
  #BM_atomic_add_to_seq_cst<int32_t>                            6.557071440111132 ns      6.526548754217108 ns     107245051
  #BM_atomic_add_to_seq_cst<int64_t>                             6.61054872108173 ns      6.581425697716481 ns     106357707
  #BM_atomic_add_to_seq_cst<float>                               7.93492707040209 ns      7.897547368104975 ns      88252950
  #BM_atomic_add_to_seq_cst<double>                             7.886038567240897 ns        7.8497197451292 ns      89287119
  #BM_atomic_add_to_relaxed<int16_t>                            5.182024355920982 ns      5.157555443739661 ns     135737011
  #BM_atomic_add_to_relaxed<int32_t>                             5.16494770312128 ns     5.1405126580046945 ns     138577686
  #BM_atomic_add_to_relaxed<int64_t>                           5.1814947733364125 ns      5.157353700881224 ns     135464777
  #BM_atomic_add_to_relaxed<float>                              7.783111893574588 ns      7.746887673515652 ns      90644491
  #BM_atomic_add_to_relaxed<double>                               7.7814935288184 ns      7.745133319828123 ns      90241753
  #BM_atomic_add_to_unordered<int16_t>                         0.3536150309955701 ns     0.3519483499999989 ns    1000000000
  #BM_atomic_add_to_unordered<int32_t>                        0.35361311800079415 ns     0.3519554869999997 ns    1000000000
  #BM_atomic_add_to_unordered<int64_t>                         0.3536399739969056 ns    0.35196829499999893 ns    1000000000
  #BM_atomic_add_to_unordered<float>                           0.7053300267486111 ns     0.7020178623833573 ns     997066161
  #BM_atomic_add_to_unordered<double>                          0.7053142521506013 ns     0.7020198464745387 ns     997044788
  #BM_gettid                                                    2.115943991086376 ns      2.106083654346265 ns     332367453
  #BM_systemTime                                                43.07716484148213 ns      42.87261607042414 ns      16329289
  #BM_thread_8_variables                                        2.821637561733614 ns      2.808209234245132 ns     249258815
  #BM_thread_local_8_variables                                  2.820507201699363 ns     2.8082240903316533 ns     249273405
  #BM_StdMutexLockUnlock                                       18.406383115576315 ns     18.319232771558767 ns      38195925
  #BM_RWMutexReadLockUnlock                                     17.04181301677912 ns     16.962061518265294 ns      41265858
  #BM_RWMutexWriteLockUnlock                                    19.11833893994331 ns      19.02850556999233 ns      36797896
  #BM_SharedMutexReadLockUnlock                                 35.60498964393801 ns      35.43688128260861 ns      19751748
  #BM_SharedMutexWriteLockUnlock                                38.00521852390459 ns      37.82714893741646 ns      18505041
  #BM_AudioUtilsMutexLockUnlock                                 33.20438439586303 ns      33.04808193472067 ns      21175687
  #BM_AudioUtilsPIMutexLockUnlock                                 34.823252358618 ns     34.659274618519255 ns      20200626
  #BM_StdMutexInitializationLockUnlock                         30.918411041550183 ns      30.77489556434292 ns      22749414
  #BM_RWMutexInitializationReadLockUnlock                       28.41113243181421 ns     28.276704201193102 ns      24754897
  #BM_RWMutexInitializationWriteLockUnlock                     30.991909358233716 ns      30.84726331659203 ns      22690659
  #BM_SharedMutexInitializationReadLockUnlock                    58.6180771593902 ns     58.344618743289224 ns      11994661
  #BM_SharedMutexInitializationWriteLockUnlock                  60.57289575308067 ns     60.291177006858376 ns      11610697
  #BM_AudioUtilsMutexInitializationLockUnlock                    45.1727720110517 ns      44.96317124907669 ns      15567973
  #BM_AudioUtilsPIMutexInitializationLockUnlock                 56.51245654873645 ns      56.24826020323107 ns      12446425
  #BM_StdMutexBlockingConditionVariable/threads:2              13005.859125798019 ns     15785.260059044813 ns         42002
  #BM_AudioUtilsMutexBlockingConditionVariable/threads:2        51395.10524573919 ns      62111.48912393159 ns         93600
  #BM_AudioUtilsPIMutexBlockingConditionVariable/threads:2     38003.326141779035 ns        48747.110842948 ns         16988
  #BM_StdMutexScopedLockUnlock/threads:1                       31.047443261227432 ns      30.90162837297469 ns      21748273
  #BM_StdMutexScopedLockUnlock/threads:2                        187.5369570007024 ns      371.9489944999989 ns       2000000
  #BM_StdMutexScopedLockUnlock/threads:4                       183.24430610750633 ns      652.1608249451782 ns       2367624
  #BM_StdMutexScopedLockUnlock/threads:8                       114.40673350319138 ns     457.55848734565944 ns       1731896
  #BM_RWMutexScopedReadLockUnlock/threads:1                     32.36649526110872 ns      32.21528625736229 ns      21668875
  #BM_RWMutexScopedReadLockUnlock/threads:2                    233.82989524907316 ns      465.2684319999982 ns       2000000
  #BM_RWMutexScopedReadLockUnlock/threads:4                     234.2368397183923 ns      876.9921508228601 ns        701220
  #BM_RWMutexScopedReadLockUnlock/threads:8                    261.81297505285545 ns     2047.9601000535506 ns        403384
  #BM_RWMutexScopedWriteLockUnlock/threads:1                    34.75660442655403 ns      34.59235428851013 ns      20236050
  #BM_RWMutexScopedWriteLockUnlock/threads:2                   233.78794333484538 ns      459.1015740364359 ns       2612646
  #BM_RWMutexScopedWriteLockUnlock/threads:4                    329.8850715993315 ns     1057.9245678854486 ns        782200
  #BM_RWMutexScopedWriteLockUnlock/threads:8                    547.4260907751869 ns      2289.554716970124 ns        685016
  #BM_SharedMutexScopedReadLockUnlock/threads:1                 67.81834135936606 ns       67.5253075922258 ns       9641336
  #BM_SharedMutexScopedReadLockUnlock/threads:2                  359.628342996484 ns      714.5713215382715 ns       1716424
  #BM_SharedMutexScopedReadLockUnlock/threads:4                354.53138343836275 ns     1245.6886813947233 ns        948032
  #BM_SharedMutexScopedReadLockUnlock/threads:8                  358.283363638594 ns      1612.215822485538 ns        431424
  #BM_SharedMutexScopedWriteLockUnlock/threads:1                74.97589649902241 ns      74.61641205682437 ns       8257929
  #BM_SharedMutexScopedWriteLockUnlock/threads:2               386.34333957740654 ns      686.9830245124164 ns       1034904
  #BM_SharedMutexScopedWriteLockUnlock/threads:4                6140.788010450615 ns     15583.110439217011 ns        403172
  #BM_SharedMutexScopedWriteLockUnlock/threads:8                3039.076686160784 ns     11077.392742135295 ns        129184
  #BM_AudioUtilsMutexScopedLockUnlock/threads:1                 66.64561062274281 ns        66.349105514304 ns      10548911
  #BM_AudioUtilsMutexScopedLockUnlock/threads:2                481.48238149951794 ns       951.586758000001 ns       2000000
  #BM_AudioUtilsMutexScopedLockUnlock/threads:4                285.27629596770197 ns      908.6821992538335 ns        737632
  #BM_AudioUtilsMutexScopedLockUnlock/threads:8                 336.3453911753518 ns     1293.2229317430258 ns        578168
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:1               67.85915888024319 ns      67.56177071445192 ns       8313616
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:2               10762.71635475132 ns     13490.832976499998 ns       2000000
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:4              17841.919923356494 ns      26260.20764057142 ns         47876
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:8               27627.09897164099 ns       36121.4640768056 ns         15728
  #BM_StdMutexReverseScopedLockUnlock/threads:1                31.179721602212002 ns     31.038040257397274 ns      21308084
  #BM_StdMutexReverseScopedLockUnlock/threads:2                 61.21498053399278 ns      116.6737022856601 ns       6378638
  #BM_StdMutexReverseScopedLockUnlock/threads:4                112.91128021867448 ns      370.7553689205596 ns       1887344
  #BM_StdMutexReverseScopedLockUnlock/threads:8                141.66823079666526 ns      551.0014334496227 ns       1566152
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:1           67.0489114081147 ns      66.73026969103846 ns       8505214
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:2         220.90466438402458 ns     416.39108014586975 ns       1243832
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:4          291.7432157916735 ns      969.6308767058123 ns       1076028
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:8         316.76478317856913 ns     1169.0109563042486 ns        586968
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:1        68.07235467128096 ns      67.79246629476496 ns       8952912
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:2        4730.325827513298 ns      5911.874809999971 ns        200000
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:4       28942.076574315968 ns      38915.04260355007 ns         23660
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:8        28147.14339638534 ns      36412.06354560159 ns         44472
  #BM_empty_while                                              0.3526294760085875 ns    0.35102246399999615 ns    1000000000

*/

// ---

template<typename Integer>
static void BM_atomic_add_equals(benchmark::State &state) {
    Integer i = 10;
    std::atomic<Integer> dst;
    while (state.KeepRunning()) {
        dst += i;
    }
    LOG(DEBUG) << __func__ << "  " << dst.load();
}

BENCHMARK(BM_atomic_add_equals<int32_t>);

template <typename T>
static void BM_atomic_add_to(benchmark::State &state, std::memory_order order) {
    int64_t i64 = 10;
    std::atomic<T> dst;
    while (state.KeepRunning()) {
        android::audio_utils::atomic_add_to(dst, i64, order);
    }
    LOG(DEBUG) << __func__ << "  " << dst.load();
}

// Avoid macro issues with the comma.
template <typename T>
static void BM_atomic_add_to_seq_cst(benchmark::State &state) {
    BM_atomic_add_to<T>(state, std::memory_order_seq_cst);
}

BENCHMARK(BM_atomic_add_to_seq_cst<int16_t>);

BENCHMARK(BM_atomic_add_to_seq_cst<int32_t>);

BENCHMARK(BM_atomic_add_to_seq_cst<int64_t>);

BENCHMARK(BM_atomic_add_to_seq_cst<float>);

BENCHMARK(BM_atomic_add_to_seq_cst<double>);

template <typename T>
static void BM_atomic_add_to_relaxed(benchmark::State &state) {
    BM_atomic_add_to<T>(state, std::memory_order_relaxed);
}

BENCHMARK(BM_atomic_add_to_relaxed<int16_t>);

BENCHMARK(BM_atomic_add_to_relaxed<int32_t>);

BENCHMARK(BM_atomic_add_to_relaxed<int64_t>);

BENCHMARK(BM_atomic_add_to_relaxed<float>);

BENCHMARK(BM_atomic_add_to_relaxed<double>);

template <typename T>
static void BM_atomic_add_to_unordered(benchmark::State &state) {
    int64_t i64 = 10;
    android::audio_utils::unordered_atomic<T> dst;
    while (state.KeepRunning()) {
        android::audio_utils::atomic_add_to(dst, i64, std::memory_order_relaxed);
    }
    LOG(DEBUG) << __func__ << "  " << dst.load();
}

BENCHMARK(BM_atomic_add_to_unordered<int16_t>);

BENCHMARK(BM_atomic_add_to_unordered<int32_t>);

BENCHMARK(BM_atomic_add_to_unordered<int64_t>);

BENCHMARK(BM_atomic_add_to_unordered<float>);

BENCHMARK(BM_atomic_add_to_unordered<double>);

// Benchmark gettid().  The mutex class uses this to get the linux thread id.
static void BM_gettid(benchmark::State &state) {
    int32_t value = 0;
    while (state.KeepRunning()) {
        value ^= android::audio_utils::gettid_wrapper();  // ensure the return value used.
    }
    ALOGD("%s: value:%d", __func__, value);
}

BENCHMARK(BM_gettid);

// Benchmark systemTime().  The mutex class uses this for timing.
static void BM_systemTime(benchmark::State &state) {
    int64_t value = 0;
    while (state.KeepRunning()) {
        value ^= systemTime();
    }
    ALOGD("%s: value:%lld", __func__, (long long)value);
}

BENCHMARK(BM_systemTime);

// Benchmark access to 8 thread local storage variables by compiler built_in __thread.
__thread volatile int tls_value1 = 1;
__thread volatile int tls_value2 = 2;
__thread volatile int tls_value3 = 3;
__thread volatile int tls_value4 = 4;
__thread volatile int tls_value5 = 5;
__thread volatile int tls_value6 = 6;
__thread volatile int tls_value7 = 7;
__thread volatile int tls_value8 = 8;

static void BM_thread_8_variables(benchmark::State &state) {
    while (state.KeepRunning()) {
        tls_value1 ^= tls_value1 ^ tls_value2 ^ tls_value3 ^ tls_value4 ^
                tls_value5 ^ tls_value6 ^ tls_value7 ^ tls_value8;
    }
    ALOGD("%s: value:%d", __func__, tls_value1);
}

BENCHMARK(BM_thread_8_variables);

// Benchmark access to 8 thread local storage variables through the
// the C/C++ 11 standard thread_local.
thread_local volatile int tlsa_value1 = 1;
thread_local volatile int tlsa_value2 = 2;
thread_local volatile int tlsa_value3 = 3;
thread_local volatile int tlsa_value4 = 4;
thread_local volatile int tlsa_value5 = 5;
thread_local volatile int tlsa_value6 = 6;
thread_local volatile int tlsa_value7 = 7;
thread_local volatile int tlsa_value8 = 8;

static void BM_thread_local_8_variables(benchmark::State &state) {
    while (state.KeepRunning()) {
        tlsa_value1 ^= tlsa_value1 ^ tlsa_value2 ^ tlsa_value3 ^ tlsa_value4 ^
                tlsa_value5 ^ tlsa_value6 ^ tlsa_value7 ^ tlsa_value8;
    }
    ALOGD("%s: value:%d", __func__, tlsa_value1);
}

BENCHMARK(BM_thread_local_8_variables);

// ---

// std::mutex is the reference mutex that we compare against.
// It is based on Bionic pthread_mutex* support.

// RWLock is a specialized Android mutex class based on
// Bionic pthread_rwlock* which in turn is based on the
// original ART shared reader mutex.

// Test shared read lock performance.
class RWReadMutex : private android::RWLock {
public:
    void lock() { readLock(); }
    bool try_lock() { return tryReadLock() == 0; }
    using android::RWLock::unlock;
};

// Test exclusive write lock performance.
class RWWriteMutex : private android::RWLock {
public:
    void lock() { writeLock(); }
    bool try_lock() { return tryWriteLock() == 0; }
    using android::RWLock::unlock;
};

// std::shared_mutex lock/unlock behavior is default exclusive.
// We subclass to create the shared reader equivalent.
//
// Unfortunately std::shared_mutex implementation can contend on an internal
// mutex with multiple readers (even with no writers), resulting in worse lock performance
// than other shared mutexes.
// This is due to the portability desire in the original reference implementation:
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2406.html#shared_mutex_imp
class SharedReadMutex : private std::shared_mutex {
public:
    void lock() { std::shared_mutex::lock_shared(); }
    bool try_lock() { return std::shared_mutex::try_lock_shared(); }
    void unlock() { std::shared_mutex::unlock_shared(); }
};

// audio_utils mutex is designed to have mutex order checking, statistics,
// deadlock detection, and priority inheritance capabilities,
// so it is higher overhead than just the std::mutex that it is based upon.
//
// audio_utils mutex without priority inheritance.
class AudioMutex : public android::audio_utils::mutex {
public:
    AudioMutex() : android::audio_utils::mutex(false /* priority_inheritance */) {}
};

// audio_utils mutex with priority inheritance.
class AudioPIMutex : public android::audio_utils::mutex {
public:
    AudioPIMutex() : android::audio_utils::mutex(true /* priority_inheritance */) {}
};

template <typename Mutex>
void MutexLockUnlock(benchmark::State& state) {
    Mutex m;
    while (state.KeepRunning()) {
        m.lock();
        m.unlock();
    }
}

static void BM_StdMutexLockUnlock(benchmark::State &state) {
    MutexLockUnlock<std::mutex>(state);
}

// Benchmark repeated mutex lock/unlock from a single thread
// using the mutex.
BENCHMARK(BM_StdMutexLockUnlock);

static void BM_RWMutexReadLockUnlock(benchmark::State& state) {
    MutexLockUnlock<RWReadMutex>(state);
}

BENCHMARK(BM_RWMutexReadLockUnlock);

static void BM_RWMutexWriteLockUnlock(benchmark::State& state) {
    MutexLockUnlock<RWWriteMutex>(state);
}

BENCHMARK(BM_RWMutexWriteLockUnlock);

static void BM_SharedMutexReadLockUnlock(benchmark::State& state) {
    MutexLockUnlock<SharedReadMutex>(state);
}

BENCHMARK(BM_SharedMutexReadLockUnlock);

static void BM_SharedMutexWriteLockUnlock(benchmark::State& state) {
    MutexLockUnlock<std::shared_mutex>(state);
}

BENCHMARK(BM_SharedMutexWriteLockUnlock);

static void BM_AudioUtilsMutexLockUnlock(benchmark::State &state) {
    MutexLockUnlock<AudioMutex>(state);
}

BENCHMARK(BM_AudioUtilsMutexLockUnlock);

static void BM_AudioUtilsPIMutexLockUnlock(benchmark::State &state) {
    MutexLockUnlock<AudioPIMutex>(state);
}

BENCHMARK(BM_AudioUtilsPIMutexLockUnlock);

// ---

template <typename Mutex>
void MutexInitializationLockUnlock(benchmark::State& state) {
    while (state.KeepRunning()) {
        Mutex m;
        m.lock();
        m.unlock();
    }
}

static void BM_StdMutexInitializationLockUnlock(benchmark::State &state) {
    MutexInitializationLockUnlock<std::mutex>(state);
}

// Benchmark repeated mutex creation then lock/unlock from a single thread
// using the mutex.
BENCHMARK(BM_StdMutexInitializationLockUnlock);

static void BM_RWMutexInitializationReadLockUnlock(benchmark::State& state) {
    MutexInitializationLockUnlock<RWReadMutex>(state);
}

BENCHMARK(BM_RWMutexInitializationReadLockUnlock);

static void BM_RWMutexInitializationWriteLockUnlock(benchmark::State& state) {
    MutexInitializationLockUnlock<RWWriteMutex>(state);
}

BENCHMARK(BM_RWMutexInitializationWriteLockUnlock);

static void BM_SharedMutexInitializationReadLockUnlock(benchmark::State& state) {
    MutexInitializationLockUnlock<SharedReadMutex>(state);
}

BENCHMARK(BM_SharedMutexInitializationReadLockUnlock);

static void BM_SharedMutexInitializationWriteLockUnlock(benchmark::State& state) {
    MutexInitializationLockUnlock<std::shared_mutex>(state);
}

BENCHMARK(BM_SharedMutexInitializationWriteLockUnlock);

static void BM_AudioUtilsMutexInitializationLockUnlock(benchmark::State &state) {
    MutexInitializationLockUnlock<AudioMutex>(state);
}

BENCHMARK(BM_AudioUtilsMutexInitializationLockUnlock);

static void BM_AudioUtilsPIMutexInitializationLockUnlock(benchmark::State &state) {
    MutexInitializationLockUnlock<AudioPIMutex>(state);
}

BENCHMARK(BM_AudioUtilsPIMutexInitializationLockUnlock);

// ---

static constexpr size_t THREADS = 2;

template <typename Mutex, typename UniqueLock, typename ConditionVariable>
class MutexBlockingConditionVariable {
    Mutex m;
    ConditionVariable cv[THREADS];
    bool wake[THREADS];

public:
    void run(benchmark::State& state) {
        const size_t local = state.thread_index();
        const size_t remote = (local + 1) % THREADS;
        if (local == 0) wake[local] = true;
        for (auto _ : state) {
            UniqueLock ul(m);
            cv[local].wait(ul, [&]{ return wake[local]; });
            wake[remote] = true;
            wake[local] = false;
            cv[remote].notify_one();
        }
        UniqueLock ul(m);
        wake[remote] = true;
        cv[remote].notify_one();
    }
};

MutexBlockingConditionVariable<std::mutex,
            std::unique_lock<std::mutex>,
            std::condition_variable> CvStd;

static void BM_StdMutexBlockingConditionVariable(benchmark::State &state) {
    CvStd.run(state);
}

// Benchmark 2 threads that use condition variables to wake each other up,
// where only one thread is active at a given time.  This benchmark
// uses mutex, unique_lock, and condition_variable.
BENCHMARK(BM_StdMutexBlockingConditionVariable)->Threads(THREADS);

MutexBlockingConditionVariable<AudioMutex,
        android::audio_utils::unique_lock,
        android::audio_utils::condition_variable> CvAu;

static void BM_AudioUtilsMutexBlockingConditionVariable(benchmark::State &state) {
    CvAu.run(state);
}

BENCHMARK(BM_AudioUtilsMutexBlockingConditionVariable)->Threads(THREADS);

MutexBlockingConditionVariable<AudioPIMutex,
        android::audio_utils::unique_lock,
        android::audio_utils::condition_variable> CvAuPI;

static void BM_AudioUtilsPIMutexBlockingConditionVariable(benchmark::State &state) {
    CvAuPI.run(state);
}

BENCHMARK(BM_AudioUtilsPIMutexBlockingConditionVariable)->Threads(THREADS);

// ---

// Benchmark scoped_lock where two threads try to obtain the
// same 2 locks with the same initial acquisition order.
// This uses std::scoped_lock.

static constexpr size_t THREADS_SCOPED = 8;

template <typename Mutex, typename ScopedLock>
class MutexScopedLockUnlock {
    const bool reverse_;
    Mutex m1_, m2_;
    int counter_ = 0;

public:
    MutexScopedLockUnlock(bool reverse = false) : reverse_(reverse) {}

    void run(benchmark::State& state) {
        const size_t index = state.thread_index();
        for (auto _ : state) {
            if (!reverse_ || index & 1) {
                ScopedLock s1(m1_, m2_);
                ++counter_;
            } else {
                ScopedLock s1(m2_, m1_);
                ++counter_;
            }
        }
    }
};

MutexScopedLockUnlock<std::mutex,
        std::scoped_lock<std::mutex, std::mutex>> ScopedStd;

static void BM_StdMutexScopedLockUnlock(benchmark::State &state) {
    ScopedStd.run(state);
}

BENCHMARK(BM_StdMutexScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<RWReadMutex,
        std::scoped_lock<RWReadMutex, RWReadMutex>> ScopedRwRead;

static void BM_RWMutexScopedReadLockUnlock(benchmark::State &state) {
    ScopedRwRead.run(state);
}

BENCHMARK(BM_RWMutexScopedReadLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<RWWriteMutex,
        std::scoped_lock<RWWriteMutex, RWWriteMutex>> ScopedRwWrite;

static void BM_RWMutexScopedWriteLockUnlock(benchmark::State &state) {
    ScopedRwWrite.run(state);
}

BENCHMARK(BM_RWMutexScopedWriteLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<SharedReadMutex,
        std::scoped_lock<SharedReadMutex, SharedReadMutex>> ScopedSharedRead;

static void BM_SharedMutexScopedReadLockUnlock(benchmark::State &state) {
    ScopedSharedRead.run(state);
}

BENCHMARK(BM_SharedMutexScopedReadLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<std::shared_mutex,
        std::scoped_lock<std::shared_mutex, std::shared_mutex>> ScopedSharedWrite;

static void BM_SharedMutexScopedWriteLockUnlock(benchmark::State &state) {
    ScopedSharedWrite.run(state);
}

BENCHMARK(BM_SharedMutexScopedWriteLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<AudioMutex,
        android::audio_utils::scoped_lock<
                AudioMutex, AudioMutex>> ScopedAu;

static void BM_AudioUtilsMutexScopedLockUnlock(benchmark::State &state) {
    ScopedAu.run(state);
}

BENCHMARK(BM_AudioUtilsMutexScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<AudioPIMutex,
        android::audio_utils::scoped_lock<
                AudioPIMutex, AudioPIMutex>> ScopedAuPI;

static void BM_AudioUtilsPIMutexScopedLockUnlock(benchmark::State &state) {
    ScopedAuPI.run(state);
}

BENCHMARK(BM_AudioUtilsPIMutexScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<std::mutex,
        std::scoped_lock<std::mutex, std::mutex>> ReverseScopedStd(true);

static void BM_StdMutexReverseScopedLockUnlock(benchmark::State &state) {
    ReverseScopedStd.run(state);
}

// Benchmark scoped_lock with odd thread having reversed scoped_lock mutex acquisition order.
// This uses std::scoped_lock.
BENCHMARK(BM_StdMutexReverseScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<AudioMutex,
        android::audio_utils::scoped_lock<
                AudioMutex, AudioMutex>> ReverseScopedAu(true);

static void BM_AudioUtilsMutexReverseScopedLockUnlock(benchmark::State &state) {
    ReverseScopedAu.run(state);
}

BENCHMARK(BM_AudioUtilsMutexReverseScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

MutexScopedLockUnlock<AudioPIMutex,
        android::audio_utils::scoped_lock<
                AudioPIMutex, AudioPIMutex>> ReverseScopedAuPI(true);

static void BM_AudioUtilsPIMutexReverseScopedLockUnlock(benchmark::State &state) {
    ReverseScopedAuPI.run(state);
}

BENCHMARK(BM_AudioUtilsPIMutexReverseScopedLockUnlock)->ThreadRange(1, THREADS_SCOPED);

static void BM_empty_while(benchmark::State &state) {

    while (state.KeepRunning()) {
        ;
    }
    ALOGD("%s", android::audio_utils::mutex::all_stats_to_string().c_str());
}

// Benchmark to see the cost of doing nothing.
BENCHMARK(BM_empty_while);

BENCHMARK_MAIN();
