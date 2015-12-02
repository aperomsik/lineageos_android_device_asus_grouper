# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This file sets variables that control the way modules are built
# thorughout the system. It should not be used to conditionally
# disable makefiles (the proper mechanism to control what gets
# included in a build is to use PRODUCT_PACKAGES in a product
# definition file).
#

# WARNING: This line must come *before* including the proprietary
# variant, so that it gets overwritten by the parent (which goes
# against the traditional rules of inheritance).
# The proprietary variant sets USE_CAMERA_STUB := false, this way
# we use the camera stub when the vendor tree isn't present, and
# the true camera library when the vendor tree is available.  Similarly,
# we set USE_PROPRIETARY_AUDIO_EXTENSIONS to true in the proprietary variant as
# well.
USE_CAMERA_STUB := true
USE_PROPRIETARY_AUDIO_EXTENSIONS := false

BOARD_HAL_STATIC_LIBRARIES := libdumpstate.grouper

TARGET_RELEASETOOLS_EXTENSIONS := device/asus/grouper

-include vendor/asus/grouper/BoardConfigVendor.mk
include device/asus/grouper/BoardConfigCommon.mk

TARGET_RECOVERY_FSTAB = device/asus/grouper/fstab.grouper

TARGET_DISABLE_ARM_PIE := true
TARGET_GLOBAL_CFLAGS += -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hardfp -gtoggle -s -DNDEBUG -march=armv7-a -mthumb -O2 -funroll-loops -mimplicit-it=always -mno-warn-deprecated -mauto-it --disable-docs -mtls-dialect=gnu2 --param l1-cache-size=32 --param l1-cache-line-size=32 --param l2-cache-size=1024 --param simultaneous-prefetches=6 --param prefetch-latency=400 -mvectorize-with-neon-quad
TARGET_GLOBAL_CPPFLAGS += -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hardfp -gtoggle -s -DNDEBUG -O2 -funroll-loops -mthumb -march=armv7-a -mimplicit-it=always -mno-warn-deprecated -mauto-it --disable-docs -mtls-dialect=gnu2 --param l1-cache-size=32 --param l1-cache-line-size=32 --param l2-cache-size=1024 --param simultaneous-prefetches=6 --param prefetch-latency=400 -mvectorize-with-neon-quad
TARGET_BOOTANIMATION_PRELOAD := true 
BOARD_SKIP_ANDROID_DOC_BUILD := true
DISABLE_DROIDDOC := true
TARGET_BOOTANIMATION_TEXTURE_CACHE := false
TARGET_ENABLE_NON_PIE_SUPPORT := true
#WITH_DEXPREOPT := true
HWUI_COMPILE_FOR_PERF := true
$(call add-product-dex-preopt-module-config,services,--compiler-filter=everything)
PRODUCT_PROPERTY_OVERRIDES += \
dalvik.vm.dex2oat-flags=--no-watch-dog \
dalvik.vm.dex2oat-filter=everything \
dalvik.vm.image-dex2oat-filter=everything
