/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef POWER_HAL_BARRIER_H
#define POWER_HAL_BARRIER_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/threads.h>

typedef android::Mutex Mutex;
typedef android::Condition Condition;

class Barrier
{
public:
    inline Barrier() : state(CLOSED) { }
    inline ~Barrier() { }
    void open() {
        Mutex::Autolock _l(lock);
        state = OPENED;
        cv.broadcast();
    }
    void close() {
        Mutex::Autolock _l(lock);
        state = CLOSED;
    }
    void wait() const {
        Mutex::Autolock _l(lock);
        while (state == CLOSED) {
            cv.wait(lock);
        }
    }
private:
    enum { OPENED, CLOSED };
    mutable     Mutex       lock;
    mutable     Condition   cv;
    volatile    int         state;
};

#endif // POWER_HAL_BARRIER_H
