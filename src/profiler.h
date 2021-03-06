/*
 * Copyright 2016 Andrei Pangin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _PROFILER_H
#define _PROFILER_H

#include <iostream>
#include "spinLock.h"
#include "symbols.h"
#include "vmEntry.h"


const int MAX_CALLTRACES    = 32768;
const int MAX_STACK_FRAMES  = 4096;
const int MAX_NATIVE_FRAMES = 128;
const int MAX_NATIVE_LIBS   = 4096;
const int CONCURRENCY_LEVEL = 16;

const int DEFAULT_FRAME_BUFFER_SIZE = 1024*1024;
const int DEFAULT_INTERVAL          = 10;
const int DEFAULT_DURATION          = 3600;
const int DEFAULT_TRACES_TO_DUMP    = 500;


typedef unsigned long long u64;

static inline int cmp64(u64 a, u64 b) {
    return a > b ? 1 : a == b ? 0 : -1;
}


class CallTraceSample {
  private:
    u64 _counter;
    int _start_frame; // Offset in frame buffer
    int _num_frames;

  public:
    static int comparator(const void* s1, const void* s2) {
        return cmp64(((CallTraceSample*)s2)->_counter, ((CallTraceSample*)s1)->_counter);
    }

    friend class Profiler;
};

class MethodSample {
  private:
    u64 _counter;
    jmethodID _method;

  public:
    static int comparator(const void* s1, const void* s2) {
        return cmp64(((MethodSample*)s2)->_counter, ((MethodSample*)s1)->_counter);
    }

    friend class Profiler;
};

class Profiler {
  private:

    // See hotspot/src/share/vm/prims/forte.cpp
    enum {
        ticks_no_Java_frame         =  0,
        ticks_no_class_load         = -1,
        ticks_GC_active             = -2,
        ticks_unknown_not_Java      = -3,
        ticks_not_walkable_not_Java = -4,
        ticks_unknown_Java          = -5,
        ticks_not_walkable_Java     = -6,
        ticks_unknown_state         = -7,
        ticks_thread_exit           = -8,
        ticks_deopt                 = -9,
        ticks_safepoint             = -10,
        ticks_skipped               = -11,
        FAILURE_TYPES               = 12
    };

    bool _running;
    u64 _samples;
    u64 _failures[FAILURE_TYPES];
    u64 _hashes[MAX_CALLTRACES];
    CallTraceSample _traces[MAX_CALLTRACES];
    MethodSample _methods[MAX_CALLTRACES];

    SpinLock _locks[CONCURRENCY_LEVEL];
    ASGCT_CallFrame _asgct_buffer[CONCURRENCY_LEVEL][MAX_STACK_FRAMES];
    jmethodID* _frame_buffer;
    int _frame_buffer_size;
    volatile int _frame_buffer_index;
    bool _frame_buffer_overflow;

    CodeCache _java_code;
    CodeCache* _native_code[MAX_NATIVE_LIBS];
    int _native_libs;

    // Seconds resolution is enough
    time_t _deadline;

    int getNativeTrace(void* ucontext, ASGCT_CallFrame* frames);
    int getJavaTrace(void* ucontext, ASGCT_CallFrame* frames, int native_frames);
    u64 hashCallTrace(int num_frames, ASGCT_CallFrame* frames);
    void storeCallTrace(int num_frames, ASGCT_CallFrame* frames);
    void copyToFrameBuffer(int num_frames, ASGCT_CallFrame* frames, CallTraceSample* trace);
    u64 hashMethod(jmethodID method);
    void storeMethod(jmethodID method);
    void checkDeadline();
    jmethodID findNativeMethod(const void* address);
    void resetSymbols();
    void setSignalHandler();
    void setTimer(long sec, long usec);
    void nonzeroSummary(std::ostream& out, const char* title, int calls, double percent);

  public:
    static Profiler _instance;

    Profiler() : 
        _running(false), 
        _frame_buffer(NULL), 
        _frame_buffer_size(DEFAULT_FRAME_BUFFER_SIZE),
        _java_code("[jvm]"),
        _native_libs(0) {
    }

    bool running() { return _running; }
    int samples() { return _samples; }

    void frameBufferSize(int size);
    void start(int interval, int duration = DEFAULT_DURATION);
    void stop();
    void summary(std::ostream& out);
    void dumpRawTraces(std::ostream& out);
    void dumpTraces(std::ostream& out, int max_traces);
    void dumpMethods(std::ostream& out);
    void recordSample(void* ucontext);

    // CompiledMethodLoad is also needed to enable DebugNonSafepoints info by default
    static void JNICALL CompiledMethodLoad(jvmtiEnv* jvmti, jmethodID method,
                                           jint code_size, const void* code_addr,
                                           jint map_length, const jvmtiAddrLocationMap* map,
                                           const void* compile_info) {
        _instance._java_code.add(code_addr, code_size, method);
    }

    static void JNICALL CompiledMethodUnload(jvmtiEnv* jvmti, jmethodID method,
                                             const void* code_addr) {
        _instance._java_code.remove(code_addr, method);
    }

    static void JNICALL DynamicCodeGenerated(jvmtiEnv* jvmti, const char* name,
                                             const void* address, jint length) {
        const char* name_copy = _instance._java_code.addString(name);
        _instance._java_code.add(address, length, name_copy);
    }
};

#endif // _PROFILER_H
