/*
 * Copyright (C) 2012 The Android Open Source Project
 * Copyright (c) 2013-2014, NVIDIA CORPORATION.  All rights reserved.
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

#include "powerhal.h"

static struct powerhal_info *pInfo;
static struct input_dev_map input_devs[] = {
		{-1, "atmel-maxtouch\n"},
		{-1, "elan-touchscreen\n"},
		{-1, "ft5x0x_ts\n"},
       };

static void tegra3_power_init(struct power_module *module)
{
    if (!pInfo)
        pInfo = (powerhal_info*)calloc(1, sizeof(powerhal_info));

    pInfo->input_devs = input_devs;
    pInfo->input_cnt = sizeof(input_devs)/sizeof(struct input_dev_map);
    common_power_init(module, pInfo);
}

static void tegra3_power_set_interactive(struct power_module *module, int on)
{
    common_power_set_interactive(module, pInfo, on);
}

static void tegra3_power_hint(struct power_module *module, power_hint_t hint,
                            void *data)
{
    common_power_hint(module, pInfo, hint, data);
}

#ifdef ANDROID_API_LP_OR_LATER
static void tegra3_set_feature(__attribute__ ((unused)) struct power_module *module, feature_t feature, __attribute__ ((unused)) int state)
{
    switch (feature) {
    case POWER_FEATURE_DOUBLE_TAP_TO_WAKE:
        ALOGW("Double tap to wake is not supported\n");
        break;
    default:
        ALOGW("Error setting the feature, it doesn't exist %d\n", feature);
        break;
    }
}
#endif

static int tegra3_power_open(__attribute__ ((unused)) const hw_module_t *module, const char *name,
                          __attribute__ ((unused)) hw_device_t **device)
{
    if (strcmp(name, POWER_HARDWARE_MODULE_ID))
        return -EINVAL;

    if (!pInfo) {
        pInfo = (powerhal_info*)calloc(1, sizeof(powerhal_info));
        power_module *dev = (power_module *)calloc(1,
                sizeof(power_module));

        if (dev) {
            /* Common hw_device_t fields */
            dev->common.tag = HARDWARE_MODULE_TAG;
#ifdef ANDROID_API_LP_OR_LATER
            dev->common.module_api_version = POWER_MODULE_API_VERSION_0_3;
#else
            dev->common.module_api_version = POWER_MODULE_API_VERSION_0_2;
#endif
            dev->common.hal_api_version = HARDWARE_HAL_API_VERSION;
            dev->init = tegra3_power_init;
            dev->setInteractive = tegra3_power_set_interactive;
            dev->powerHint = tegra3_power_hint;
#ifdef ANDROID_API_LP_OR_LATER
            dev->setFeature = tegra3_set_feature;
#endif
            *device = (hw_device_t*)dev;
        } else
            return -ENOMEM;

        if(pInfo)
            common_power_open(pInfo);
        else
            return -ENOMEM;
    }

    return 0;
}

static struct hw_module_methods_t power_module_methods = {
    open: tegra3_power_open,
};

struct power_module HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
#ifdef ANDROID_API_LP_OR_LATER
        module_api_version: POWER_MODULE_API_VERSION_0_3,
#else
        module_api_version: POWER_MODULE_API_VERSION_0_2,
#endif
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: POWER_HARDWARE_MODULE_ID,
        name: "Tegra 3 Power HAL",
        author: "NVIDIA",
        methods: &power_module_methods,
        dso: NULL,
        reserved: {0},
    },

    init: tegra3_power_init,
    setInteractive: tegra3_power_set_interactive,
    powerHint: tegra3_power_hint,
#ifdef ANDROID_API_LP_OR_LATER
    setFeature: tegra3_set_feature,
#endif
};
