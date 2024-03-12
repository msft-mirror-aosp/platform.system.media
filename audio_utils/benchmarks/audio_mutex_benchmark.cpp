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
  #BM_atomic_add_equals<int32_t>                               6.508700118072382 ns      6.471633177192451 ns     108110486
  #BM_atomic_add_to_seq_cst<int16_t>                           6.557658152513349 ns      6.526665108542128 ns     107252873
  #BM_atomic_add_to_seq_cst<int32_t>                            6.61304199453549 ns       6.58175539524565 ns     106351923
  #BM_atomic_add_to_seq_cst<int64_t>                           6.557521711571485 ns     6.5265363568644625 ns     107250668
  #BM_atomic_add_to_seq_cst<float>                             7.895243222524512 ns      7.858297243207844 ns      89394951
  #BM_atomic_add_to_seq_cst<double>                            7.931688495474578 ns      7.893971885098797 ns      88653486
  #BM_atomic_add_to_relaxed<int16_t>                           5.140386288993005 ns      5.116383769230237 ns     135131188
  #BM_atomic_add_to_relaxed<int32_t>                           5.181670175781189 ns      5.157418005923224 ns     135724804
  #BM_atomic_add_to_relaxed<int64_t>                           5.161260548149761 ns      5.136776648952849 ns     135135216
  #BM_atomic_add_to_relaxed<float>                             7.786417198158838 ns      7.749791796134465 ns      90646732
  #BM_atomic_add_to_relaxed<double>                            7.760358404716961 ns      7.723992286938152 ns      90644677
  #BM_gettid                                                   2.116039491081284 ns      2.106033253650779 ns     332358395
  #BM_systemTime                                              43.074033150581585 ns       42.8699911242381 ns      16328739
  #BM_thread_8_variables                                      2.8214796173366734 ns     2.8081271094521703 ns     249273547
  #BM_thread_local_8_variables                                 2.819987500327649 ns      2.808149311074747 ns     249278495
  #BM_StdMutexLockUnlock                                      18.155770972784783 ns     18.070903999828232 ns      38747264
  #BM_RWMutexReadLockUnlock                                    16.12456214871892 ns      16.04901684644192 ns      43612414
  #BM_RWMutexWriteLockUnlock                                   19.14824893658628 ns      19.05893391346091 ns      36725255
  #BM_SharedMutexReadLockUnlock                                39.54155074347332 ns      39.35497456828369 ns      17788418
  #BM_SharedMutexWriteLockUnlock                               41.58785205766037 ns      41.39323040198865 ns      16911078
  #BM_AudioUtilsMutexLockUnlock                                66.56918230215399 ns      66.25544975244046 ns      10562911
  #BM_AudioUtilsPIMutexLockUnlock                              67.02589961630612 ns      66.70819768056897 ns      10493090
  #BM_StdMutexInitializationLockUnlock                        29.544903877103074 ns     29.406544528057406 ns      23801319
  #BM_RWMutexInitializationReadLockUnlock                      26.91749522594829 ns     26.802654591541785 ns      26123567
  #BM_RWMutexInitializationWriteLockUnlock                     30.20599678894913 ns      30.06422812747118 ns      23284596
  #BM_SharedMutexInitializationReadLockUnlock                 58.070478136125395 ns      57.79511704041489 ns      12111671
  #BM_SharedMutexInitializationWriteLockUnlock                 59.36722820827075 ns      59.08875400469678 ns      11843905
  #BM_AudioUtilsMutexInitializationLockUnlock                  85.04952357479699 ns      84.65093492146583 ns       8269839
  #BM_AudioUtilsPIMutexInitializationLockUnlock                83.32953114993384 ns       82.9411400506946 ns       8440765
  #BM_StdMutexBlockingConditionVariable/threads:2             20067.186478012434 ns     25402.779402102544 ns         54792
  #BM_AudioUtilsMutexBlockingConditionVariable/threads:2       48417.40553370931 ns      58220.13591731267 ns         23220
  #BM_AudioUtilsPIMutexBlockingConditionVariable/threads:2     48724.90563264992 ns      59858.82489342454 ns         15482
  #BM_StdMutexScopedLockUnlock/threads:1                       33.58821991644139 ns      33.41913176098606 ns      16058919
  #BM_StdMutexScopedLockUnlock/threads:2                      356.67886764843007 ns      707.8318856903202 ns       4625680
  #BM_StdMutexScopedLockUnlock/threads:4                      130.45108549886208 ns      447.1268742499998 ns       4000000
  #BM_StdMutexScopedLockUnlock/threads:8                       139.0823761208755 ns      541.9088026721488 ns       1362200
  #BM_RWMutexScopedReadLockUnlock/threads:1                    32.33613871803748 ns     32.194204614295046 ns      21710272
  #BM_RWMutexScopedReadLockUnlock/threads:2                   160.47792160732033 ns      319.3012639397403 ns       2095986
  #BM_RWMutexScopedReadLockUnlock/threads:4                   217.21087383931467 ns      861.2673855686197 ns        839892
  #BM_RWMutexScopedReadLockUnlock/threads:8                   232.19586516883186 ns     1831.4409709220026 ns        491368
  #BM_RWMutexScopedWriteLockUnlock/threads:1                   33.49908180449042 ns      33.34195684310611 ns      21010780
  #BM_RWMutexScopedWriteLockUnlock/threads:2                    286.096410842338 ns       564.599202114389 ns       2485068
  #BM_RWMutexScopedWriteLockUnlock/threads:4                   451.7913123512162 ns     1601.6332793492106 ns       1931432
  #BM_RWMutexScopedWriteLockUnlock/threads:8                  417.50240217790537 ns     1678.8585405353656 ns        794072
  #BM_SharedMutexScopedReadLockUnlock/threads:1                67.65354544884363 ns      67.37498338520537 ns       9133426
  #BM_SharedMutexScopedReadLockUnlock/threads:2               370.22816132765433 ns      735.4710534035784 ns       1322608
  #BM_SharedMutexScopedReadLockUnlock/threads:4                298.7991937078523 ns     1015.8674764877635 ns        991824
  #BM_SharedMutexScopedReadLockUnlock/threads:8               359.17200914091643 ns     1500.1318202480697 ns        615960
  #BM_SharedMutexScopedWriteLockUnlock/threads:1               73.40224842642553 ns      73.06218848168656 ns       8616869
  #BM_SharedMutexScopedWriteLockUnlock/threads:2               502.8427941278981 ns      909.1756670594543 ns        599122
  #BM_SharedMutexScopedWriteLockUnlock/threads:4              2322.7325028106275 ns      6083.585590040707 ns        313436
  #BM_SharedMutexScopedWriteLockUnlock/threads:8               4948.555700826256 ns     15412.772486815033 ns        373152
  #BM_AudioUtilsMutexScopedLockUnlock/threads:1               147.60580533538862 ns     146.97151308638587 ns       4062848
  #BM_AudioUtilsMutexScopedLockUnlock/threads:2                5409.319112352385 ns     10729.084861761592 ns        728090
  #BM_AudioUtilsMutexScopedLockUnlock/threads:4                630.9403610213494 ns     1866.9171243841429 ns        579688
  #BM_AudioUtilsMutexScopedLockUnlock/threads:8                612.9153996947896 ns     2167.0654441098654 ns        417104
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:1             148.94249680999073 ns      148.3061023465011 ns       4387722
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:2              3537.898640072271 ns      4287.604650248743 ns        356196
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:4             13969.834843789307 ns      19572.29615170118 ns         28688
  #BM_AudioUtilsPIMutexScopedLockUnlock/threads:8             30652.264078729862 ns      40000.50360617244 ns         23848
  #BM_StdMutexReverseScopedLockUnlock/threads:1                31.34740304135938 ns     31.200396418488175 ns      21854682
  #BM_StdMutexReverseScopedLockUnlock/threads:2                54.06016658620641 ns      103.2554157873692 ns       5317694
  #BM_StdMutexReverseScopedLockUnlock/threads:4                169.8661622311813 ns      592.4042833246494 ns       3209096
  #BM_StdMutexReverseScopedLockUnlock/threads:8               156.65913206788008 ns       604.623918327717 ns       1742672
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:1        147.51456839840807 ns     146.73295356311675 ns       4395816
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:2        2425.8992549948744 ns      4812.346055000001 ns        200000
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:4         453.8639331349259 ns     1256.0567649999934 ns        400000
  #BM_AudioUtilsMutexReverseScopedLockUnlock/threads:8         635.5625220561735 ns      2294.725433768965 ns        356872
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:1       148.7079480412097 ns      148.0359150267745 ns       4188943
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:2      14037.435207752424 ns     17829.977469499998 ns       2000000
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:4      20098.127750043204 ns      26126.68207500001 ns         40000
  #BM_AudioUtilsPIMutexReverseScopedLockUnlock/threads:8      28805.264783022852 ns      38780.66452074406 ns         16776
  #BM_empty_while                                              0.352701456999057 ns    0.35104016500000057 ns    1000000000

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
