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

#include <sstream>
#include "profiler.h"


extern "C" JNIEXPORT void JNICALL
Java_one_profiler_AsyncProfiler_start0(JNIEnv* env, jobject unused, jint interval) {
    Profiler::_instance.start(interval ? interval : DEFAULT_INTERVAL);
}

extern "C" JNIEXPORT void JNICALL
Java_one_profiler_AsyncProfiler_stop0(JNIEnv* env, jobject unused) {
    Profiler::_instance.stop();
}

extern "C" JNIEXPORT jint JNICALL
Java_one_profiler_AsyncProfiler_getSamples(JNIEnv* env, jobject unused) {
    return Profiler::_instance.samples();
}

extern "C" JNIEXPORT jstring JNICALL
Java_one_profiler_AsyncProfiler_dumpTraces0(JNIEnv* env, jobject unused, jint max_traces) {
    std::ostringstream out;
    Profiler::_instance.summary(out);
    Profiler::_instance.dumpTraces(out, max_traces ? max_traces : DEFAULT_TRACES_TO_DUMP);
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_one_profiler_AsyncProfiler_dumpMethods0(JNIEnv* env, jobject unused) {
    std::ostringstream out;
    Profiler::_instance.summary(out);
    Profiler::_instance.dumpMethods(out);
    return env->NewStringUTF(out.str().c_str());
}
