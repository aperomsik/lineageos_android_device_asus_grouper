/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
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
#define LOG_TAG "powerHAL::common"

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "powerhal_utils.h"
#include "powerhal.h"

#define TOTAL_CPUS 4
#define LOW_POWER_MAX_FREQ "620000"
#define LOW_POWER_MIN_FREQ "204000"
#define NORMAL_MIN_FREQ "204000"
#define NORMAL_MAX_FREQ "1300000"

static char *cpu_path_min[] = {
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq",
};

static char *cpu_path_max[] = {
    "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq",
};

static bool freq_set[TOTAL_CPUS];
static bool low_power_mode = false;
static pthread_mutex_t low_power_mode_lock = PTHREAD_MUTEX_INITIALIZER;

static int get_input_count(void)
{
    int i = 0;
    int ret;
    char path[80];
    char name[50];

    while(1)
    {
        snprintf(path, sizeof(path), "/sys/class/input/input%d/name", i);
        ret = access(path, F_OK);
        if (ret < 0)
            break;
        memset(name, 0, 50);
        sysfs_read(path, name, 50);
        ALOGI("input device id:%d present with name:%s", i++, name);
    }
    return i;
}

static void find_input_device_ids(struct powerhal_info *pInfo)
{
    int i = 0;
    int status;
    int count = 0;
    char path[80];
    char name[MAX_CHARS];

    while (1)
    {
        snprintf(path, sizeof(path), "/sys/class/input/input%d/name", i);
        if (access(path, F_OK) < 0)
            break;
        else {
            memset(name, 0, MAX_CHARS);
            sysfs_read(path, name, MAX_CHARS);
            for (int j = 0; j < pInfo->input_cnt; j++) {
                status = (-1 == pInfo->input_devs[j].dev_id)
                    && (0 == strncmp(name,
                    pInfo->input_devs[j].dev_name, MAX_CHARS));
                if (status) {
                    ++count;
                    pInfo->input_devs[j].dev_id = i;
                    ALOGI("find_input_device_ids: %d %s",
                        pInfo->input_devs[j].dev_id,
                        pInfo->input_devs[j].dev_name);
                }
            }
            ++i;
        }

        if (count == pInfo->input_cnt)
            break;
    }
}

static int check_hint(struct powerhal_info *pInfo, power_hint_t hint, uint64_t *t)
{
    struct timespec ts;
    uint64_t time;

    if (hint >= MAX_POWER_HINT_COUNT) {
        ALOGE("Invalid power hint: 0x%x", hint);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts);
    time = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;

    if (pInfo->hint_time[hint] && pInfo->hint_interval[hint] &&
        (time - pInfo->hint_time[hint] < pInfo->hint_interval[hint]))
        return -1;

    *t = time;

    return 0;
}

static bool is_available_frequency(struct powerhal_info *pInfo, int freq)
{
    int i;

    for(i = 0; i < pInfo->num_available_frequencies; i++) {
        if(pInfo->available_frequencies[i] == freq)
            return true;
    }

    return false;
}

void common_power_open(struct powerhal_info *pInfo)
{
    int i;
    int size = 256;
    char *pch;

    if (0 == pInfo->input_devs || 0 == pInfo->input_cnt)
        pInfo->input_cnt = get_input_count();
    else
        find_input_device_ids(pInfo);

    // Initialize timeout poker
    Barrier readyToRun;
    pInfo->mTimeoutPoker = new TimeoutPoker(&readyToRun);
    readyToRun.wait();

    // Read available frequencies
    char *buf = (char*)malloc(sizeof(char) * size);
    memset(buf, 0, size);
    sysfs_read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
               buf, size);

    // Determine number of available frequencies
    pch = strtok(buf, " ");
    pInfo->num_available_frequencies = -1;
    while(pch != NULL)
    {
        pch = strtok(NULL, " ");
        pInfo->num_available_frequencies++;
    }

    // Store available frequencies in a lookup array
    pInfo->available_frequencies = (int*)malloc(sizeof(int) * pInfo->num_available_frequencies);
    sysfs_read("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
               buf, size);
    pch = strtok(buf, " ");
    for(i = 0; i < pInfo->num_available_frequencies; i++)
    {
        pInfo->available_frequencies[i] = atoi(pch);
        pch = strtok(NULL, " ");
    }

    pInfo->max_frequency = pInfo->available_frequencies[pInfo->num_available_frequencies - 1];

    // Store LP cluster max frequency
    sysfs_read("/sys/module/cpu_tegra3/parameters/idle_top_freq",
                buf, size);
    pInfo->lp_max_frequency = atoi(buf);

    pInfo->interaction_boost_frequency = pInfo->lp_max_frequency;
    pInfo->animation_boost_frequency = pInfo->lp_max_frequency;

    for (i = 0; i < pInfo->num_available_frequencies; i++)
    {
        if (pInfo->available_frequencies[i] >= 1200000) {
            pInfo->interaction_boost_frequency = pInfo->available_frequencies[i];
            break;
        }
    }

    for (i = 0; i < pInfo->num_available_frequencies; i++)
    {
        if (pInfo->available_frequencies[i] >= 1000000) {
            pInfo->animation_boost_frequency = pInfo->available_frequencies[i];
            break;
        }
    }

    // Initialize hint intervals in usec
    //
    // Set the interaction timeout to be slightly shorter than the duration of
    // the interaction boost so that we can maintain is constantly during
    // interaction.
    pInfo->hint_interval[POWER_HINT_INTERACTION] = 90000;

    free(buf);
}

void common_power_init(__attribute__ ((unused)) struct power_module *module,
        struct powerhal_info *pInfo)
{
    common_power_open(pInfo);

    pInfo->ftrace_enable = get_property_bool("nvidia.hwc.ftrace_enable", false);

    // set default intelliactive definitions
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/above_hispeed_delay", 20000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse", 1);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse_duration", 80000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/go_hispeed_load", 90);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/hispeed_freq", 1000000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/io_is_busy", 1);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/min_sample_time", 20000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/sampling_down_factor", 40000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/sync_freq", 640000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/target_loads", 85);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/timer_rate", 10000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/timer_slack", 40000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/up_threshold_any_cpu_freq", 860000);
    sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/up_threshold_any_cpu_load", 80);

    // Boost to max frequency on initialization to decrease boot time
    pInfo->mTimeoutPoker->requestPmQosTimed("/dev/cpu_freq_min", pInfo->max_frequency,
                                     s2ns(60));
    pInfo->mTimeoutPoker->requestPmQosTimed("/dev/min_online_cpus", DEFAULT_MAX_ONLINE_CPUS,
                                     s2ns(60));
    ALOGI("Boosting cpu_freq_min to %d for 60 seconds to make boot faster", pInfo->max_frequency);
}

void common_power_set_interactive(__attribute__ ((unused)) struct power_module *module,
        struct powerhal_info *pInfo, int on)
{
    int i;
    int dev_id;
    char path[80];
    const char* state = (0 == on)?"0":"1";
    const char* lp_state = (on)?"1":"0";
    const char* gov = (on == 0)?"intelliactive":"intelliactive";

    sysfs_write("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", gov);
    ALOGI("Setting scaling_governor to %s", gov);

    sysfs_write("/sys/module/cpu_tegra3/parameters/no_lp", lp_state);
    ALOGI("Setting low power cluster %s", lp_state);

    if (on) {
        ALOGI("Setting boost %s", state);
        sysfs_write("/sys/kernel/cluster/active", "G");
        sysfs_write("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse", state);
        ALOGI("Screen is on, setting aggressive values for intelliactive governor");
        sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse", 1);
        sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/io_is_busy", 1);
        sysfs_write(cpu_path_min[0], NORMAL_MIN_FREQ);
        sysfs_write(cpu_path_max[0], NORMAL_MAX_FREQ);
    } else {
        ALOGI("Screen is off, setting relaxed values for intelliactive governor");
        sysfs_write("/sys/kernel/cluster/active", "LP");
        sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse", 0);
        sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/io_is_busy", 0);
        sysfs_write(cpu_path_min[0], LOW_POWER_MIN_FREQ);
        sysfs_write(cpu_path_max[0], LOW_POWER_MAX_FREQ);
    }

    if (0 != pInfo) {
        for (i = 0; i < pInfo->input_cnt; i++) {
            if (0 == pInfo->input_devs)
                dev_id = i;
            else if (-1 == pInfo->input_devs[i].dev_id)
                continue;
            else
                dev_id = pInfo->input_devs[i].dev_id;
            snprintf(path, sizeof(path), "/sys/class/input/input%d/enabled", dev_id);
            if (!access(path, W_OK)) {
                if (0 == on)
                    ALOGI("Disabling input device:%d", dev_id);
                else
                    ALOGI("Enabling input device:%d", dev_id);
                sysfs_write(path, state);
            }
        }
    }

    if ( on == 0 ) {
        const char* display_mode = "/sys/devices/virtual/switch/tegradc.0/state";
        char mode[9] = "";

        memset(mode, 0, 9);
        usleep(4000); // Sleep a bit for the filepaths to change and permissions to be set
        sysfs_read(display_mode, mode, 9);
        ALOGD("Display mode is currently %s", mode);
        if ( strncmp(mode,"offline",7) == 0 ) {
            ALOGI("Screen is off, setting relaxed values for intelliactive governor");
        } else {
            ALOGI("Screen is on, setting aggressive values for intelliactive governor");
        }
    }

}

void common_power_hint(__attribute__ ((unused)) struct power_module *module,
        struct powerhal_info *pInfo, power_hint_t hint, __attribute__ ((unused)) void *data)
{
    int len, cpu, ret;
    uint64_t t;

    if (!pInfo)
        return;

    if (check_hint(pInfo, hint, &t) < 0)
        return;

    switch (hint) {
    case POWER_HINT_VSYNC:
        break;
    case POWER_HINT_INTERACTION:
        ALOGI("POWER_HINT_INTERACTION");
        pthread_mutex_lock(&low_power_mode_lock);
        if (low_power_mode) {
           sysfs_write("/sys/module/cpu_tegra3/parameters/no_lp", 0);
           sysfs_write("/sys/kernel/cluster/active", "G");
        }
        //sysfs_write_int("/sys/devices/system/cpu/cpufreq/intelliactive/boostpulse", 1);
        if (pInfo->ftrace_enable) {
            sysfs_write("/sys/kernel/debug/tracing/trace_marker", "Start POWER_HINT_INTERACTION\n");
        }
        low_power_mode = false;
        pthread_mutex_unlock(&low_power_mode_lock);
        break;
    case POWER_HINT_LOW_POWER:
        ALOGI("POWER_HINT_LOW_POWER");
        pthread_mutex_lock(&low_power_mode_lock);
        if (!low_power_mode) {
           sysfs_write_int("/sys/module/cpu_tegra3/parameters/no_lp", -1);
           sysfs_write("/sys/kernel/cluster/active", "LP");
        }
        low_power_mode = true;
        pthread_mutex_unlock(&low_power_mode_lock);
        //pthread_mutex_lock(&low_power_mode_lock);
	//common_power_set_interactive(module, pInfo, 0);
        //if (pInfo->ftrace_enable) {
        //    sysfs_write("/sys/kernel/debug/tracing/trace_marker", "Start POWER_HINT_LOW_POWER\n");
        //}
        //low_power_mode = true;
        //sysfs_write(cpu_path_min[0], LOW_POWER_MIN_FREQ);
        //sysfs_write(cpu_path_max[0], LOW_POWER_MAX_FREQ);
        //pthread_mutex_unlock(&low_power_mode_lock);
	break;
    default:
        ALOGE("Unknown power hint: 0x%x", hint);
        break;
    }

    pInfo->hint_time[hint] = t;
}
