/*
 * Copyright (c) 2011-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#ifndef POWER_HAL_TIMEOUT_POKER_H
#define POWER_HAL_TIMEOUT_POKER_H

#include <stdint.h>
#include <sys/types.h>

#include <android/looper.h>
#include <utils/threads.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Looper.h>
#include <utils/Log.h>

#include "barrier.h"

#define COMMAND_SIZE 20
#define NODE_TYPE_DEFAULT 0
#define NODE_TYPE_PRIORITY 1

//It seems redundant to need both this message queue
//And the IPC threads message queue
//But I didn't see an easy way to
//run an event after a timeout on the IPC threads

using namespace android;

static int createConstraintCommand(char* command, int size, int priority, int max, int min);

class TimeoutPoker {
private:
    class PokeHandler;


public:
    TimeoutPoker(Barrier* readyToRun);

    // Interface for requests that do not have a priority parameter.
    // Uses /dev/[cpu_freq_max, cpu_freq_min, max_online_cpus,
    // min_onlins_cpus, gpu_freq_max, gpu_freq_min] sysnodes which
    // default to priority of 50.
    int createPmQosHandle(const char* filename, int val);
    int requestPmQos(const char* filename, int val);
    void requestPmQosTimed(const char* filename, int val, nsecs_t timeoutNs);

    // Interface for requests with a priority parameter.
    // Uses /dev/constraint_[cpu_freq, onlines_cpus, gpu_freq] sysnodes.
    // Command format: "max min priority timeoutMs"
    int createPmQosHandle(const char* filename, int priority, int max, int min);
    int requestPmQos(const char* filename, int priority, int max, int min);
    void requestPmQosTimed(const char* filename, int priority, int max, int min, nsecs_t timeoutNs);

private:

    class QueuedEvent {
    public:
        virtual ~QueuedEvent() {}
        QueuedEvent() { }

        virtual void run(PokeHandler * const thiz) = 0;
    };

    class PmQosOpenTimedEvent : public QueuedEvent {
    public:
        virtual ~PmQosOpenTimedEvent() {}
        PmQosOpenTimedEvent(const char* node,
                int val,
                nsecs_t timeout) :
            node(node),
            val(val),
            timeout(timeout) {
            type = NODE_TYPE_DEFAULT;
            priority = -1;
            max = -1;
            min = -1;
        }

        PmQosOpenTimedEvent(const char* node,
                int priority,
                int max,
                int min,
                nsecs_t timeout) :
            node(node),
            priority(priority),
            max(max),
            min(min),
            timeout(timeout) {
            type = NODE_TYPE_PRIORITY;
            val = 0;
        }

        virtual void run(PokeHandler * const thiz) {
            if (type == NODE_TYPE_PRIORITY) {
                thiz->openPmQosTimed(node, priority, max, min, timeout);
            } else {
                thiz->openPmQosTimed(node, val, timeout);
            }
        }

    private:
        const char* node;
        int val;
        int priority;
        int max;
        int min;
        int type;
        nsecs_t timeout;
    };

    class PmQosOpenHandleEvent : public QueuedEvent {
    public:
        virtual ~PmQosOpenHandleEvent() {}
        PmQosOpenHandleEvent(const char* node,
                int val,
                int* outFd,
                Barrier* done) :
            node(node),
            val(val),
            outFd(outFd),
            done(done) {
            type = NODE_TYPE_DEFAULT;
            priority = -1;
            max = -1;
            min = -1;
        }

        PmQosOpenHandleEvent(const char* node,
                int priority,
                int max,
                int min,
                int* outFd,
                Barrier* done) :
            node(node),
            priority(priority),
            max(max),
            min(min),
            outFd(outFd),
            done(done) {
            type = NODE_TYPE_PRIORITY;
            val = 0;
        }

        virtual void run(PokeHandler * const thiz) {
            if (type == NODE_TYPE_PRIORITY) {
                *outFd = thiz->createHandleForPmQosRequest(node, priority, max, min);
            } else {
                *outFd = thiz->createHandleForPmQosRequest(node, val);
            }
            done->open();
        }

    private:
        const char* node;
        int val;
        int priority;
        int max;
        int min;
        int type;
        int* outFd;
        Barrier* done;
    };

    class TimeoutEvent : public QueuedEvent {
    public:
        virtual ~TimeoutEvent() {}
        TimeoutEvent(int pmQosFd) : pmQosFd(pmQosFd) {}

        virtual void run(PokeHandler * const thiz) {
            thiz->timeoutRequest(pmQosFd);
        }

    private:
        int pmQosFd;
    };

    void pushEvent(QueuedEvent* event);

    class PokeHandler : public MessageHandler {
        class LooperThread : public Thread {
            private:
                Barrier* mReadyToRun;
            public:
                sp<Looper> mLooper;
                virtual bool threadLoop();
                LooperThread(Barrier* readyToRun) :
                    mReadyToRun(readyToRun) {}
                virtual status_t readyToRun();
        };
    public:

        sp<LooperThread> mWorker;

        KeyedVector<unsigned int, QueuedEvent*> mQueuedEvents;

        virtual void handleMessage(const Message& msg);
        PokeHandler(TimeoutPoker* poker, Barrier* readyToRun);
        int generateNewKey(void);
        void sendEventDelayed(nsecs_t delay, QueuedEvent* ev);
        int listenForHandleToCloseFd(int handle, int fd);
        QueuedEvent* removeEventByKey(int key);
        int createHandleForFd(int fd);
        void timeoutRequest(int fd);

        void openPmQosTimed(const char* fileName, int val, nsecs_t timeout);
        int createHandleForPmQosRequest(const char* filename, int val);
        int openPmQosNode(const char* filename, int val);

        void openPmQosTimed(const char* fileName, int priority, int max, int min, nsecs_t timeout);
        int createHandleForPmQosRequest(const char* filename, int priority, int max, int min);
        int openPmQosNode(const char* filename, int prioirity, int max, int min);

    private:
        TimeoutPoker* mPoker;
        int mKey;

        bool mSpamRefresh;
        mutable Mutex mEvLock;
    };

    sp<PokeHandler> mPokeHandler;
};

#endif
