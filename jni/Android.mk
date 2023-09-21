LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= resetprop.cpp base.cpp persist.cpp
LOCAL_MODULE:= resetprop
LOCAL_LDLIBS           := -llog -landroid
LOCAL_STATIC_LIBRARIES := libsystemproperties libnanopb
LOCAL_CFLAGS := -std=c++17

# LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE:= libnanopb
LOCAL_C_INCLUDES := $(LOCAL_PATH)/nanopb
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_SRC_FILES := \
    nanopb/pb_common.c \
    nanopb/pb_decode.c \
    nanopb/pb_encode.c
include $(BUILD_STATIC_LIBRARY)

# include system_properties/Android.mk
include sp.mk
