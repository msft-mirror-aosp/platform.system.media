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
  #BM_atomic_add_equals<int32_t>                                6.490418995069907 ns     6.4717376481357896 ns     108158957
  #BM_atomic_add_to_seq_cst<int16_t>                           6.5491215252883315 ns      6.528080020397028 ns     107223462
  #BM_atomic_add_to_seq_cst<int32_t>                           6.6043085052910895 ns       6.58277029519354 ns     106339895
  #BM_atomic_add_to_seq_cst<int64_t>                            6.547130657683545 ns      6.527710835284538 ns     107241300
  #BM_atomic_add_to_seq_cst<float>                              7.886748664549371 ns     7.8608841749954355 ns      89038641
  #BM_atomic_add_to_seq_cst<double>                             7.935562180977917 ns      7.910317678915859 ns      88467659
  #BM_atomic_add_to_relaxed<int16_t>                            5.144169800499881 ns      5.127361036326905 ns     137048029
  #BM_atomic_add_to_relaxed<int32_t>                            5.168846899607784 ns       5.15352435703851 ns     135676634
  #BM_atomic_add_to_relaxed<int64_t>                            5.156732436798179 ns      5.141098657660172 ns     136413594
  #BM_atomic_add_to_relaxed<float>                              7.763552575883229 ns      7.740082913372091 ns      90622053
  #BM_atomic_add_to_relaxed<double>                             7.760723400931919 ns      7.734912038002161 ns      90618849
  #BM_atomic_add_to_unordered<int16_t>                         0.3533747960000255 ns     0.3520660080000013 ns    1000000000
  #BM_atomic_add_to_unordered<int32_t>                         0.3534268799999154 ns      0.352105915000001 ns    1000000000
  #BM_atomic_add_to_unordered<int64_t>                        0.35323697900003026 ns    0.35204362999999894 ns    1000000000
  #BM_atomic_add_to_unordered<float>                           0.7043684281594664 ns     0.7021647364411598 ns     988700453
  #BM_atomic_add_to_unordered<double>                           0.704450251294842 ns     0.7021509129361374 ns     996984194
  #BM_gettid                                                   2.1128518266669247 ns      2.106438363375118 ns     332324191
  #BM_systemTime                                                43.50481860259643 ns     43.373881730104316 ns      16213103
  #BM_thread_8_variables                                       2.8174664322723015 ns     2.8088585587229447 ns     249169901
  #BM_thread_local_8_variables                                 2.8176008559183345 ns     2.8088503601840658 ns     249223823
  #BM_StdMutexLockUnlock                                        20.51083054083145 ns     20.372680732323307 ns      34365643
  #BM_RWMutexReadLockUnlock                                    17.182708241218037 ns      17.10085471231418 ns      40872349
  #BM_RWMutexWriteLockUnlock                                    20.01395996116509 ns     19.912307155808747 ns      35403322
  #BM_SharedMutexReadLockUnlock                                 39.34289759177089 ns     39.214633183208534 ns      17848256
  #BM_SharedMutexWriteLockUnlock                                42.42260135644499 ns      42.25185684039555 ns      16568333
  #BM_AudioUtilsMutexLockUnlock                                 32.59607274485956 ns      32.48981289778165 ns      21549504
  #BM_AudioUtilsPIMutexLockUnlock                               33.79847568429067 ns       33.6964229192697 ns      20775880
  #BM_StdMutexInitializationLockUnlock                          30.36758342133683 ns     30.254101880283233 ns      23141887
  #BM_RWMutexInitializationReadLockUnlock                       27.28375660870322 ns     27.196487591970985 ns      25738012
  #BM_RWMutexInitializationWriteLockUnlock                      30.20724264266599 ns     30.096879362472333 ns      23256594
  #BM_SharedMutexInitializationReadLockUnlock                   57.43815201585343 ns      57.27383821891096 ns      12222096
  #BM_SharedMutexInitializationWriteLockUnlock                  59.42673061824289 ns      59.25235124842115 ns      11814362
  #BM_AudioUtilsMutexInitializationLockUnlock                   46.10038716918369 ns     45.953988368973455 ns      15233909
  #BM_AudioUtilsPIMutexInitializationLockUnlock                 50.73553222492994 ns     50.574418947890834 ns      13835334
  #BM_StdMutexBlockingConditionVariable/threads:2               11523.72384534072 ns     12714.605659025783 ns         58632
  #BM_AudioUtilsMutexBlockingConditionVariable/threads:2        9338.361496790618 ns     11206.032771535578 ns         74760
  #BM_AudioUtilsPIMutexBlockingConditionVariable/threads:2     12430.610334229705 ns     13459.017326162135 ns         52060
  #BM_StdMutexScopedLockUnlock/threads:1                       33.534067204276546 ns      33.40309483152711 ns      20796027
  #BM_StdMutexScopedLockUnlock/threads:2                        269.1759952499524 ns       533.950398499998 ns       2000000
  #BM_StdMutexScopedLockUnlock/threads:4                        90.18870335515196 ns      271.3231852294451 ns       2269488
  #BM_StdMutexScopedLockUnlock/threads:8                       121.03213508602038 ns      451.7371193384729 ns       2448632
  #BM_RWMutexScopedReadLockUnlock/threads:1                     32.11047130691962 ns      31.96092065619549 ns      21757351
  #BM_RWMutexScopedReadLockUnlock/threads:2                    117.73928731993787 ns     230.31362984633367 ns       2348992
  #BM_RWMutexScopedReadLockUnlock/threads:4                     220.8538545474783 ns      858.6430804361402 ns        949424
  #BM_RWMutexScopedReadLockUnlock/threads:8                     217.2344705376624 ns     1528.7949547499559 ns        460552
  #BM_RWMutexScopedWriteLockUnlock/threads:1                    34.76444514474894 ns     34.665961723712094 ns      20194069
  #BM_RWMutexScopedWriteLockUnlock/threads:2                   303.41208949994325 ns      603.2115715000019 ns       2000000
  #BM_RWMutexScopedWriteLockUnlock/threads:4                    298.4931931843524 ns       916.926215593706 ns       1571660
  #BM_RWMutexScopedWriteLockUnlock/threads:8                   432.74492906249407 ns     1240.5567937500045 ns        800000
  #BM_SharedMutexScopedReadLockUnlock/threads:1                 70.04048550107358 ns       69.8046640694218 ns       9059342
  #BM_SharedMutexScopedReadLockUnlock/threads:2                357.07506909046754 ns      709.4210754541601 ns       1482834
  #BM_SharedMutexScopedReadLockUnlock/threads:4                336.03568074383156 ns      1087.821794679974 ns        989168
  #BM_SharedMutexScopedReadLockUnlock/threads:8                 343.4415500594684 ns     1423.0045686060148 ns        870944
  #BM_SharedMutexScopedWriteLockUnlock/threads:1                77.31578352815413 ns      77.00259046212362 ns       8135228
  #BM_SharedMutexScopedWriteLockUnlock/threads:2                356.1377498778198 ns      627.7192368534169 ns       1218796
  #BM_SharedMutexScopedWriteLockUnlock/threads:4               2206.5972784481546 ns       5390.78073569482 ns        770700
  #BM_SharedMutexScopedWriteLockUnlock/threads:8                2643.145098618517 ns      7265.627503497389 ns       1012184
  #BM_AudioUtilsMutexScopedLockUnlock/threads:1                 68.37942831761342 ns      68.16332511845363 ns       8684647
  #BM_AudioUtilsMutexScopedLockUnlock/threads:2                 439.5642884199026 ns      868.7699421475584 ns       1605118
  #BM_AudioUtilsMutexScopedLockUnlock/threads:4                 321.1245397453114 ns     1025.1737506853917 ns       2203128
  #BM_AudioUtilsMutexScopedLockUnlock/threads:8                302.42947515758783 ns     1176.8521985370544 ns       1262112
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:1               69.87225800700081 ns      69.64552224576019 ns       8994051
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:2               4420.777346513025 ns      5456.967229338184 ns        265756
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:4              1506.8638396645179 ns      1927.406805542472 ns        424360
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:8               25030.96209476646 ns      27871.63623561846 ns         33376
  #BM_StdMutexReverseScopedLockUnlock/threads:1                 33.47593087477488 ns      33.37508010876382 ns      20550186
  #BM_StdMutexReverseScopedLockUnlock/threads:2                198.84388250011398 ns     385.92393400000117 ns       2000000
  #BM_StdMutexReverseScopedLockUnlock/threads:4                 93.50488264641875 ns      276.2069913615782 ns       3951648
  #BM_StdMutexReverseScopedLockUnlock/threads:8                110.50842131360572 ns      378.4212902611287 ns       2141768
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:1           68.2132503060489 ns      68.01976601705918 ns       9013905
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:2         223.03285165273516 ns      424.2072166440236 ns       1879738
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:4         264.11614886066064 ns      743.2390829429721 ns       1815416
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:8          274.5291393750193 ns     1015.9050412499973 ns        800000
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:1        69.50784383771779 ns      69.31317329009033 ns       8408894
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:2        631.9418303245776 ns      790.5849174679049 ns       1417388
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:4       12829.762531245593 ns     15402.261100000063 ns         40000
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:8        24954.85928430851 ns     26994.147940851126 ns         28944
  #BM_empty_while                                              0.3522347409998474 ns    0.35108219199999985 ns    1000000000

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
