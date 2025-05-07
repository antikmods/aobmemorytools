LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := memory
LOCAL_SRC_FILES := main.cpp
LOCAL_CPPFLAGS  += -std=c++17
LOCAL_LDLIBS    := -llog
include $(BUILD_EXECUTABLE)

