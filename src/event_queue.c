#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
//#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include "types.h"
#include "logging.h"
#include "timing.h"
#include <assert.h>

static SDL_threadID event_queue_lock = 0;

// try acquire the spin lock, returns true if the lock was acquired,
// false otherwise.
static bool evqu_lock_try(SDL_threadID* ptr) {
    SDL_threadID expected = 0;
    SDL_threadID desired = SDL_GetThreadID(NULL);
    return __atomic_compare_exchange (ptr, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
}

// lock implementation using atomic compare and exchange
// The thread trying to acquire the lock "spins",
// sleeping for 1 millisecond and trying again.
static void evqu_lock(SDL_threadID* ptr) {
//    printf("evqu_lock {"); fflush(stdout);
    while (!evqu_lock_try(ptr)) {
        if (*ptr == SDL_GetThreadID(NULL)) {
            error_printf("evqu_lock recursive locking is unsupported\n");
            // bug: state is uncertain terminate execution immediately
            exit(EXIT_FAILURE);
        }
        sleep_micro_seconds(100);
    }
//    printf("evqu_lock }\n"); fflush(stdout);
}

// release previously acquired spin lock.
static void evqu_unlock(SDL_threadID* ptr) {
//printf("evqu_unlock {"); fflush(stdout);
    SDL_threadID expected = SDL_GetThreadID(NULL);
    SDL_threadID desired = 0;
    if (__atomic_compare_exchange (ptr, &expected, &desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
//printf("evqu_unlock }\n"); fflush(stdout);
        return;
    }
    error_printf("evqu_unlock: invoked by thread not holding the lock\n");
    // bug: state is uncertain terminate execution immediately
    exit(EXIT_FAILURE);
}

#define QSIZE 1024
static SDL_Event evq[QSIZE];
static volatile int head=0;
static volatile int tail=0;

#define NUDGE(x) (((x) + 1) % QSIZE)

bool evt_q_put(SDL_Event* evt) {
    SDL_Event* poke = NULL;
    bool ok = false;
    evqu_lock(&event_queue_lock);
    int next_head = NUDGE(head);
    if (next_head != tail) {
        poke = evq+head;
        memcpy(poke, evt, sizeof(*evt));
//        printf("put %d %d %d %p\n", head, evt->type, evq[head].type, evq+head);
        ok = true;
        head = next_head;
    } else {
        error_printf("event queue is full\n");
    }
    evqu_unlock(&event_queue_lock);
    return ok;
}

bool evt_q_peek(SDL_Event *evt) {
    bool ok = false;
    SDL_Event* peek = NULL;
    if (tail != head) {
        evqu_lock(&event_queue_lock);
        if (tail != head) {
            peek = evq+tail;
            memcpy(evt, peek, sizeof(*evt));
            peek = evq+tail;
            ok = true;
        }
        evqu_unlock(&event_queue_lock);
    }
    return ok;
}


bool evt_q_get(SDL_Event* evt) {
    SDL_Event* peek = NULL;
    bool ok = false;
    if (tail != head) {
        evqu_lock(&event_queue_lock);
        if (tail != head) {
            peek = evq+tail;
            memcpy(evt, peek, sizeof(*evt));
//            void *tmp = memcpy(evt, peek, sizeof(*evt));
//            printf("get %d %d %d %p %p %p\n", tail, evt->type, evq[tail].type, evq+tail, evt, tmp);
            ok = true;
            tail = NUDGE(tail);
        }
        evqu_unlock(&event_queue_lock);
    }
    return ok;
}

bool evt_q_try_put(SDL_Event* evt, unsigned try_count, int64_t sleepmicros) {
    bool ok = false;
    while(try_count && !ok) {
        if (evqu_lock_try(&event_queue_lock)) {
            int next_head = (head + 1) % QSIZE;
            if (next_head != tail) {
                memcpy(evq+head, evt, sizeof(*evt));
                ok = true;
                try_count=0;
                head = next_head;
            } else {
                error_printf("evt_q_try_put: queue is full\n");
            }
            evqu_unlock(&event_queue_lock);
        } else {
            error_printf("evt_q_try_put: lock not available\n");
            if (--try_count) {
                sleep_micro_seconds(sleepmicros);
            }
        }
    }
    return ok;
}
