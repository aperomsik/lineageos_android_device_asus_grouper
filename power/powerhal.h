/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef COMMON_POWER_HAL_H
#define COMMON_POWER_HAL_H

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "powerhal_utils.h"
#include "timeoutpoker.h"
#include <semaphore.h>
#include <stdlib.h>

#define MAX_CHARS 32
#define MAX_INPUT_DEV_COUNT 12
#define MAX_USE_CASE_STRING_SIZE 80

#define MAX_POWER_HINT_COUNT 12

#define DEFAULT_MIN_ONLINE_CPUS     2
#define DEFAULT_MAX_ONLINE_CPUS     4
#define DEFAULT_FREQ                700

#define POWER_CAP_PROP "persist.sys.NV_PBC_PWR_LIMIT"
#define SLEEP_INTERVAL_SECS 1

struct input_dev_map {
    int dev_id;
    const char* dev_name;
};

struct powerhal_info {
    TimeoutPoker* mTimeoutPoker;

    int *available_frequencies;
    int num_available_frequencies;

    /* Maximum LP CPU frequency */
    int lp_max_frequency;

    /* Maximum CPU frequency */
    int max_frequency;

    int interaction_boost_frequency;
    int animation_boost_frequency;

    bool ftrace_enable;

    /* Number of devices requesting Power HAL service */
    int input_cnt;

    /* Holds input devices */
    struct input_dev_map* input_devs;

    /* Time last hint was sent - in usec */
    uint64_t hint_time[MAX_POWER_HINT_COUNT];
    uint64_t hint_interval[MAX_POWER_HINT_COUNT];

    /* waiting condvar regular hints thread */
    pthread_cond_t wait_cond;

    /* waiting mutex regular hints thread */
    pthread_mutex_t wait_mutex;

    /* regular hints thread handle */
    pthread_t regular_hints_thread;

    /* regular hints thread handle */
    bool exit_hints_thread;

    /* Features on platform */
    struct {
        bool fan;
    } features;

    /* File descriptors used for hints and app profiles */
    struct {
        int interactive_max_cpus;
    } fds;

};

/* Opens power hw module */
void common_power_open(struct powerhal_info *pInfo);

/* Power management setup action at startup.
 * Such as to set default cpufreq parameters.
 */
void common_power_init(struct power_module *module, struct powerhal_info *pInfo);

/* Power management action,
 * upon the system entering interactive state and ready for interaction,
 * often with UI devices
 * OR
 * non-interactive state the system appears asleep, displayi/touch usually turned off.
*/
void common_power_set_interactive(struct power_module *module,
                                    struct powerhal_info *pInfo, int on);

/* PowerHint called to pass hints on power requirements, which
 * may result in adjustment of power/performance parameters of the
 * cpufreq governor and other controls.
*/
void common_power_hint(struct power_module *module, struct powerhal_info *pInfo,
                            power_hint_t hint, void *data);

#endif  //COMMON_POWER_HAL_H

