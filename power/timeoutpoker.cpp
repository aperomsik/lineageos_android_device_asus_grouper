/*
 * Copyright (c) 2011-2013 NVIDIA Corporation.  All Rights Reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and
 * proprietary rights in and to this software and related documentation.  Any
 * use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation
 * is strictly prohibited.
 */
#include "timeoutpoker.h"
#include <fcntl.h>

#undef LOG_TAG
#define LOG_TAG "powerHAL::TimeoutPoker"

enum {
    CLIENT_FD,
    SERVER_FD
};

TimeoutPoker::TimeoutPoker(Barrier* readyToRun)
{
    mPokeHandler = new PokeHandler(this, readyToRun);
}

//Called usually from IPC thread
void TimeoutPoker::pushEvent(QueuedEvent* event)
{
    mPokeHandler->sendEventDelayed(0, event);
}

int TimeoutPoker::PokeHandler::createHandleForFd(int fd)
{
    int pipefd[2];
    int res = pipe(pipefd);
    if (res) {
        ALOGE("unabled to create handle for fd");
        close(fd);
        return -1;
    }

    listenForHandleToCloseFd(pipefd[SERVER_FD], fd);
    if (res) {
        close(fd);
        close(pipefd[SERVER_FD]);
        close(pipefd[CLIENT_FD]);
        return -1;
    }

    return pipefd[CLIENT_FD];
}

int TimeoutPoker::PokeHandler::openPmQosNode(const char* filename, int val)
{
    int pm_qos_fd = open(filename, O_RDWR);;
    if (pm_qos_fd < 0) {
        ALOGE("unable to open pm_qos file for %s: %s", filename, strerror(errno));
        return -1;
    }
    write(pm_qos_fd, &val, sizeof(val));
    return pm_qos_fd;
}

int TimeoutPoker::PokeHandler::openPmQosNode(const char* filename, int priority, int max, int min)
{
    int pm_qos_fd = open(filename, O_RDWR);;
    if (pm_qos_fd < 0) {
        ALOGE("unable to open pm_qos file for %s: %s", filename, strerror(errno));
        return -1;
    }
    char command[COMMAND_SIZE];
    int size = createConstraintCommand((char*)command, COMMAND_SIZE, priority, max, min);

    write(pm_qos_fd, command, size);
    return pm_qos_fd;
}

int TimeoutPoker::PokeHandler::createHandleForPmQosRequest(const char* filename, int val)
{
    int fd = openPmQosNode(filename, val);
    if (fd < 0) {
        return -1;
    }

    return createHandleForFd(fd);
}

int TimeoutPoker::PokeHandler::createHandleForPmQosRequest(const char* filename, int priority, int max, int min)
{
    int fd = openPmQosNode(filename, priority, max, min);
    if (fd < 0) {
        return -1;
    }

    return createHandleForFd(fd);
}

int TimeoutPoker::createPmQosHandle(const char* filename,
        int val)
{
    Barrier done;
    int ret;
    pushEvent(new PmQosOpenHandleEvent(
                filename, val, &ret, &done));

    done.wait();
    return ret;
}

void TimeoutPoker::requestPmQosTimed(const char* filename,
        int val, nsecs_t timeout)
{
    pushEvent(new PmQosOpenTimedEvent(
                filename, val, timeout));
    ALOGV("starting requestPmQosTimed, mNodeName %s, mQosValue %d, timeout %d milliseconds",
            filename, val, int(ns2ms(timeout)));
}

int TimeoutPoker::requestPmQos(const char* filename, int val)
{
    int pm_qos_fd = open(filename, O_RDWR);
    if (pm_qos_fd < 0) {
        ALOGE("unable to open pm_qos file for %s: %s", filename, strerror(errno));
        return -1;
    }
    write(pm_qos_fd, &val, sizeof(val));
    return pm_qos_fd;
}

int TimeoutPoker::createPmQosHandle(const char* filename,
        int priority, int max, int min)
{
    Barrier done;
    int ret;
    pushEvent(new PmQosOpenHandleEvent(
                filename, priority, max, min, &ret, &done));

    done.wait();
    return ret;
}

void TimeoutPoker::requestPmQosTimed(const char* filename,
        int priority, int max, int min, nsecs_t timeout)
{
    pushEvent(new PmQosOpenTimedEvent(
                filename, priority, max, min, timeout));
}

int TimeoutPoker::requestPmQos(const char* filename, int priority, int max, int min)
{
    int pm_qos_fd = open(filename, O_RDWR);
    if (pm_qos_fd < 0) {
        ALOGE("unable to open pm_qos file for %s: %s", filename, strerror(errno));
        return -1;
    }

    char command[COMMAND_SIZE];
    int size = createConstraintCommand((char*)command, COMMAND_SIZE, priority, max, min);

    write(pm_qos_fd, command, size);
    return pm_qos_fd;
}

/*
 * PokeHandler
 */
void TimeoutPoker::PokeHandler::sendEventDelayed(
        nsecs_t delay, TimeoutPoker::QueuedEvent* ev) {
    Mutex::Autolock _l(mEvLock);

    int key = generateNewKey();
    mQueuedEvents.add(key, ev);
    mWorker->mLooper->sendMessageDelayed(delay, this, key);
}

TimeoutPoker::QueuedEvent*
TimeoutPoker::PokeHandler::removeEventByKey(
int what) {
    Mutex::Autolock _l(mEvLock);

    // msg.what contains a key to retrieve the message parameters
    TimeoutPoker::QueuedEvent* e =
        mQueuedEvents.valueFor(what);
    mQueuedEvents.removeItem(what);
    return e;
}

int TimeoutPoker::PokeHandler::generateNewKey(void)
{
    return mKey++;
}

TimeoutPoker::PokeHandler::PokeHandler(TimeoutPoker* poker, Barrier* readyToRun) :
    mPoker(poker),
    mKey(0),
    mSpamRefresh(false)
{
    mWorker = new LooperThread(readyToRun);
    mWorker->run("TimeoutPoker::PokeHandler::LooperThread", PRIORITY_FOREGROUND);
    readyToRun->wait();
}

void TimeoutPoker::PokeHandler::handleMessage(const Message& msg)
{
    assert(!mQueuedEvents->isEmpty());

    // msg.what contains a key to retrieve the message parameters
    TimeoutPoker::QueuedEvent* e =
        removeEventByKey(msg.what);

    if (!e)
        return;

    e->run(this);

    delete e;
}

void TimeoutPoker::PokeHandler::openPmQosTimed(const char* filename,
        int val, nsecs_t timeout)
{
    int fd = openPmQosNode(filename, val);
    if (fd < 0) {
        return;
    }

    sendEventDelayed(timeout, new TimeoutEvent(fd));
}

void TimeoutPoker::PokeHandler::openPmQosTimed(const char* filename,
        int priority, int max, int min,  nsecs_t timeout)
{
    int fd = openPmQosNode(filename, priority, max, min);
    if (fd < 0) {
        return;
    }

    sendEventDelayed(timeout, new TimeoutEvent(fd));
}


void TimeoutPoker::PokeHandler::timeoutRequest(int fd)
{
    close(fd);
}

status_t TimeoutPoker::PokeHandler::LooperThread::readyToRun()
{
    mLooper = Looper::prepare(0);
    if (mLooper == 0)
        return NO_MEMORY;
    mReadyToRun->open();
    return NO_ERROR;
}

bool TimeoutPoker::PokeHandler::LooperThread::threadLoop()
{
   int res = mLooper->pollAll(99999);
   if (res == ALOOPER_POLL_ERROR)
       ALOGE("Poll returned an error!");
    return true;
}

class CallbackContext {
public:
    CallbackContext(sp<Looper> looper, int fd) : looper(looper), fd(fd) {}

    sp<Looper> looper;
    int fd;
};

static int pipeCloseCb(int handle, int events, void* data)
{
    CallbackContext* ctx = (CallbackContext*)data;

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {

        ctx->looper->removeFd(handle);
        close(handle);
        close(ctx->fd);
        delete ctx;
        return 0;
    }
    return 1;
}

//Reverse arity of result to match call-site usage
int TimeoutPoker::PokeHandler::listenForHandleToCloseFd(int handle, int fd)
{
    //This func is threadsafe
    return !mWorker->mLooper->addFd(handle, ALOOPER_POLL_CALLBACK,
            ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP,
            pipeCloseCb, new CallbackContext(mWorker->mLooper, fd));
}

int createConstraintCommand(char* command, int size, int priority, int max, int min) {
    snprintf(command, size, "%d %d %d 0", max, min, priority);
    return strlen(command);
}
