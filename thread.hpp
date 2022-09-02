//
// Copyright (C) 2005-2015 Oleg Polivets. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#pragma once

#include <vector>

#ifdef WIN32
#    include <windows.h>
#    include <process.h>
#else
#    include <pthread.h>
#    include <unistd.h>
#    include <errno.h>
#    include <limits.h>
#    include <sys/time.h>
#    include <sys/syscall.h>
#endif

namespace op {

// ///////////////////////////////////////////////////////////////////////// //

class mutex {
public:
#ifdef WIN32
    typedef CRITICAL_SECTION MUTEX_T;
    mutex()        { InitializeCriticalSection(&mMutex); }
    ~mutex()       { DeleteCriticalSection(&mMutex);     }
    void lock()    { EnterCriticalSection(&mMutex);      }
    void unlock()  { LeaveCriticalSection(&mMutex);      }
    bool trylock() { return (TryEnterCriticalSection(&mMutex) == TRUE); }
#else
    typedef pthread_mutex_t MUTEX_T;
    mutex()        { pthread_mutex_init(&mMutex, NULL);  }
    ~mutex()       { pthread_mutex_destroy(&mMutex);     }
    void lock()    { pthread_mutex_lock(&mMutex);        }
    void unlock()  { pthread_mutex_unlock(&mMutex);      }
    bool trylock() { return (pthread_mutex_trylock(&mMutex) != EBUSY); }
#endif
    class lockguard {
    public:
        lockguard(mutex & m) : mMutex(m) { mMutex.lock(); }
        ~lockguard() { mMutex.unlock(); }
    private:
        mutex & mMutex;
        lockguard(lockguard &);
        lockguard & operator=(lockguard &);
    };

    class dtorunlock {
    public:
        dtorunlock(mutex & m) : mMutex(m) {}
        ~dtorunlock() { mMutex.unlock(); }
    private:
        mutex & mMutex;
        dtorunlock(dtorunlock &);
        dtorunlock & operator=(dtorunlock &);
    };

private:
    MUTEX_T  mMutex;
    mutex (const mutex &);
    mutex& operator= (const mutex &);
};

// ///////////////////////////////////////////////////////////////////////// //

class thread {
public:
    thread()
        : m_running(false)
        , m_timeout(0)
        , m_id(0)
    {
    }

    virtual ~thread() {}

    unsigned int id() const {
        return m_id;
    }

    void stop() {
        m_running = false;
    }

    bool isRunning() const {
        return m_running;
    }

    void start() {
        if (isRunning()) return;
        m_id = 0;
        m_running = true;
        op::thread::run_thread(thread::thread_routine, this, 0);
        op::thread::sleep(0);
    }

    void setSleeppingTimeout(unsigned secs) {
        m_timeout = secs;
    }

    void sleepping() {
        for (unsigned i = 0; (i < m_timeout) && isRunning(); ++i) {
            op::thread::sleep(1000);
        }
    }

#ifdef WIN32
    typedef void (__cdecl * THREAD_ROUTINE)(void *);
    static unsigned int self_id() {
        return GetCurrentThreadId();
    }
    static void sleep(unsigned int ms) {
        Sleep(ms);
    }
    static int run_thread(THREAD_ROUTINE func, void *param, unsigned stack_size) {
        return _beginthread(func, stack_size, param);
    }
#else
    typedef void* (* THREAD_ROUTINE)(void *);
    static unsigned int self_id() {
        return syscall(SYS_gettid);
    }
    static void sleep(unsigned int msecs) {
        struct timeval tv;
        gettimeofday(&tv, 0);
        struct timespec ti;

        ti.tv_nsec = (tv.tv_usec + (msecs % 1000) * 1000) * 1000;
        ti.tv_sec = tv.tv_sec + (msecs / 1000) + (ti.tv_nsec / 1000000000);
        ti.tv_nsec %= 1000000000;

        pthread_mutex_t mtx;
        pthread_cond_t cnd;

        pthread_mutex_init(&mtx, 0);
        pthread_cond_init(&cnd, 0);

        pthread_mutex_lock(&mtx);
        (void) pthread_cond_timedwait(&cnd, &mtx, &ti);
        pthread_mutex_unlock(&mtx);

        pthread_cond_destroy(&cnd);
        pthread_mutex_destroy(&mtx);
    }
    static int run_thread(THREAD_ROUTINE func, void *param, unsigned stack_size) {
        pthread_attr_t tattr;
        pthread_t tid;
        if (stack_size < PTHREAD_STACK_MIN) {
            stack_size = 0; // by default
        }
        pthread_attr_t *ptattr = 
            (pthread_attr_setstacksize(&tattr, stack_size) == 0 ? &tattr : NULL);
        if (pthread_create(&tid, ptattr, func, param)) {
            return 0;
        }
        pthread_detach(tid);
        return tid;
    }
#endif

protected:
    volatile bool m_running;

    virtual void routine() = 0;

private:
    unsigned m_timeout;
    unsigned int m_id;

    static
#ifdef WIN32
    void
#else
    void*
#endif
    thread_routine(void * param) {
        thread * th = static_cast<thread*>(param);
        th->m_id = op::thread::self_id();
        th->routine();
        th->m_id = 0;
        th->m_running = false;
#ifndef WIN32
        return 0;
#endif
    }
};

template <class T> class thread_with_params : public thread {
public:
    typedef T param_type;

    virtual ~thread_with_params() {}

    param_type & param() { 
        return m_param;
    }

    const T & param() const {
        return m_param;
    }

    void stop() {
        m_running = false;
        op::thread::sleep(1000);
    }

protected:
    T m_param;
};

// ///////////////////////////////////////////////////////////////////////// //

template <class T> class thread_pool {
public:
    explicit thread_pool(unsigned count = 0) {
        for (unsigned i = 0; i < count; ++i)
            this->append(new T);
    }

    ~thread_pool() {
        join();

        m_mutex.lock();
        for (unsigned i = 0, ie = m_pool.size(); i < ie; ++i) {
            if (m_pool[i]->isRunning()) {
                m_pool[i]->stop();
                op::thread::sleep(1000);
            }
            delete m_pool[i];
        }
        m_pool.clear();
        m_mutex.unlock();
    }

    T * append(T * th) {
        m_mutex.lock();
        m_pool.push_back(th);
        m_mutex.unlock();
        return th;
    }

    T * fetch() {
        m_mutex.lock();
        T * th = NULL;
        for(unsigned i = 0, ie = m_pool.size(); i < ie; ++i) {
            if (m_pool[i]->isRunning()) continue;
            th = m_pool[i];
            break;
        }
        m_mutex.unlock();
        return th;
    }

    void join() {
        bool all_done = false;
        do {
            op::thread::sleep(1000);
            all_done = true;
            m_mutex.lock();
            for (unsigned i = 0, ie = m_pool.size(); i < ie; ++i) {
                if (m_pool[i]->isRunning()) {
                    all_done = false;
                    break;
                }
            }
            m_mutex.unlock();
        } while (!all_done);
    }

    void stopAll() {
        m_mutex.lock();
        for (unsigned i = 0, ie = m_pool.size(); i < ie; ++i)
            m_pool[i]->stop();
        m_mutex.unlock();
    }

private:
    std::vector< T * > m_pool;
    mutex m_mutex;
};

} // namespace op
