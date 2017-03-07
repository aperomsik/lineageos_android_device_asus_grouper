#!/system/bin/sh

# Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.

# set_hwui_params.sh -- set android hwui parameters on boot time
#
# This script reads the default display's resolution and the system memory size
# to set appropriate hwui parameters if the overriden value from build.prop does
# not already present in the device. The parameters should be tuned for the production
# devices but this script will give a coarse estimate

MEMINFO="/proc/meminfo"
FB0_MODES="/sys/class/graphics/fb0/modes"

property_set ()
{
    VALUE=`setprop $1 $2`
    echo $VALUE
}

property_get ()
{
    VALUE=`getprop $1`
    echo $VALUE
}

property_set_if_none()
{
    VALUE=$(property_get $1)
    if [ -n $VALUE ] || [ "$VALUE" == "" ];
    then
        $(property_set $1 $2)
    fi
}

get_total_system_memory ()
{
    if [ -f $MEMINFO ]; then
        # /proc/meminfo's fist line looks like below
        # MemTotal:       16412328 kB
        #so matching second column will give us
        # the total memory available on the system
        IFS=' '
        MEMINFO_STR=`cat $MEMINFO`
        set $MEMINFO_STR
        echo "$2"
    else
        echo "0"
    fi
}

get_default_display_pixel_count()
{
    # try to read fb0's display resolution from sysfs
    if [ -f $FB0_MODES ]; then
        # mode string look like 'U:1080x1920p-60 U:1080x1920p-53'
        IFS=' '
        MODES_STR=`cat $FB0_MODES`

        # extract the first entry
        set $MODES_STR
        MODES_STR="$1"

        # get the last mode from the list.
        # assumption is that the panel resolutions are all same

        # Extract substring between ':' and "x"
        XRES="${MODES_STR##*:}"
        XRES="${XRES%%x*}"

        # Extract substring after 'x' until non digit char
        YRES="${MODES_STR##*x}"
        YRES="${YRES%-*}"
        YRES="${YRES%?}"

        echo $((XRES*YRES))
    else
        echo "0"
    fi
}

# 720p      ~= 922K pixels
# 1080p     ~= 2.07M pixels
# 1920x1200 ~= 2.3M pixels
# 1440p     ~= 2.76M pixels
# 2560x1440 ~= 4.1M pixels
# 2160p(4k) ~= 8.29M pixels

PIXEL_COUNT=$(get_default_display_pixel_count)
TOTAL_SYS_MEMSIZE=$(get_total_system_memory)

if [ "$PIXEL_COUNT" -le "1000000" ]; then
    property_set_if_none "ro.hwui.texture_cache_size" "24.0"
    property_set_if_none "ro.hwui.layer_cache_size" "16.0"
    property_set_if_none "ro.hwui.gradient_cache_size" "0.5"
    property_set_if_none "ro.hwui.path_cache_size" "8.0"
    property_set_if_none "ro.hwui.shape_cache_size" "1.0"
    property_set_if_none "ro.hwui.drop_shadow_cache_size" "2.0"
    property_set_if_none "ro.hwui.text_small_cache_width" "1024"
    property_set_if_none "ro.hwui.text_small_cache_height" "256"
    property_set_if_none "ro.hwui.text_large_cache_width" "2048"
    property_set_if_none "ro.hwui.text_large_cache_height" "512"
elif [ "$PIXEL_COUNT" -le "3000000" ]; then
    property_set_if_none "ro.hwui.texture_cache_size" "48.0"
    property_set_if_none "ro.hwui.layer_cache_size" "32.0"
    property_set_if_none "ro.hwui.gradient_cache_size" "0.8"
    property_set_if_none "ro.hwui.path_cache_size" "24.0"
    property_set_if_none "ro.hwui.shape_cache_size" "3.0"
    property_set_if_none "ro.hwui.drop_shadow_cache_size" "4.0"
    property_set_if_none "ro.hwui.text_small_cache_width" "1024"
    property_set_if_none "ro.hwui.text_small_cache_height" "512"
    property_set_if_none "ro.hwui.text_large_cache_width" "2048"
    property_set_if_none "ro.hwui.text_large_cache_height" "1024"
elif [ "$PIXEL_COUNT" -le "5000000" ]; then
    property_set_if_none "ro.hwui.texture_cache_size" "72.0"
    property_set_if_none "ro.hwui.layer_cache_size" "48.0"
    property_set_if_none "ro.hwui.gradient_cache_size" "1.0"
    property_set_if_none "ro.hwui.path_cache_size" "40.0"
    property_set_if_none "ro.hwui.shape_cache_size" "5.0"
    property_set_if_none "ro.hwui.drop_shadow_cache_size" "6.0"
    property_set_if_none "ro.hwui.text_small_cache_width" "1024"
    property_set_if_none "ro.hwui.text_small_cache_height" "1024"
    property_set_if_none "ro.hwui.text_large_cache_width" "2048"
    property_set_if_none "ro.hwui.text_large_cache_height" "1024"
else
    property_set_if_none "ro.hwui.texture_cache_size" "96.0"
    property_set_if_none "ro.hwui.layer_cache_size" "64.0"
    property_set_if_none "ro.hwui.gradient_cache_size" "1.2"
    property_set_if_none "ro.hwui.path_cache_size" "60.0"
    property_set_if_none "ro.hwui.shape_cache_size" "6.0"
    property_set_if_none "ro.hwui.drop_shadow_cache_size" "8.0"
    property_set_if_none "ro.hwui.text_small_cache_width" "1024"
    property_set_if_none "ro.hwui.text_small_cache_height" "1024"
    property_set_if_none "ro.hwui.text_large_cache_width" "2048"
    property_set_if_none "ro.hwui.text_large_cache_height" "1024"
fi

# checking if the system memory size is 1G or less.
if [ "$TOTAL_SYS_MEMSIZE" -le "1048576" ]; then
    property_set_if_none "ro.hwui.texture_cache_flushrate" "0.6"
else
    property_set_if_none "ro.hwui.texture_cache_flushrate" "0.4"
fi
