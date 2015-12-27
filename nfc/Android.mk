# Copyright (C) 2011 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := nfc.grouper
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SRC_FILES := nfc_hw.c
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -D$(TARGET_DEVICE)

include $(BUILD_SHARED_LIBRARY)

########################################
# libnfc_jni for nxp stack
########################################

LOCAL_PATH := packages/apps/Nfc/nxp/jni/

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    com_android_nfc_NativeLlcpConnectionlessSocket.cpp \
    com_android_nfc_NativeLlcpServiceSocket.cpp \
    com_android_nfc_NativeLlcpSocket.cpp \
    com_android_nfc_NativeNfcManager.cpp \
    com_android_nfc_NativeNfcTag.cpp \
    com_android_nfc_NativeP2pDevice.cpp \
    com_android_nfc_list.cpp \
    com_android_nfc.cpp

LOCAL_C_INCLUDES += \
    $(JNI_H_INCLUDE) \
    external/libnfc-nxp/src \
    external/libnfc-nxp/inc \
    libcore/include

LOCAL_SHARED_LIBRARIES := \
    libnativehelper \
    libcutils \
    libutils \
    liblog \
    libnfc \
    libhardware

LOCAL_MODULE := libnfc_jni
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

########################################
# NXP Stack Configuration
########################################

LOCAL_PATH:= packages/apps/Nfc/

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        $(call all-java-files-under, src)

LOCAL_SRC_FILES += \
        $(call all-java-files-under, nxp)

LOCAL_PACKAGE_NAME := NfcNxp
LOCAL_CERTIFICATE := platform

LOCAL_JNI_SHARED_LIBRARIES  := libnfc_jni

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

include $(LOCAL_PATH)/nxp/Android.mk

########################################
# NfcNxp Tests
########################################

LOCAL_PATH:= packages/apps/Nfc/tests

include $(CLEAR_VARS)

# We only want this apk build for tests.
LOCAL_MODULE_TAGS := tests

LOCAL_JAVA_LIBRARIES := android.test.runner

# Include all test java files.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := NfcNxpTests
LOCAL_CERTIFICATE := platform

LOCAL_INSTRUMENTATION_FOR := NfcNxp

include $(BUILD_PACKAGE)
