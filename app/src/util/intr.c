#include "intr.h"

#include "util/log.h"

#include <assert.h>

bool
sc_intr_init(struct sc_intr *intr) {
    bool ok = sc_mutex_init(&intr->mutex);
    if (!ok) {
        LOGE("Could not init intr mutex");
        return false;
    }

    intr->socket = SC_SOCKET_NONE;
    intr->process = SC_PROCESS_NONE;

    atomic_store(&intr->interrupted, false);

    return true;
}

bool
sc_intr_set_socket(struct sc_intr *intr, sc_socket socket) {
    assert(intr->process == SC_PROCESS_NONE);

    sc_mutex_lock(&intr->mutex);
    bool interrupted = intr->interrupted;
    if (!interrupted) {
        intr->socket = socket;
    }
    sc_mutex_unlock(&intr->mutex);

    return !interrupted;
}

bool
sc_intr_set_process(struct sc_intr *intr, sc_pid pid) {
    assert(intr->socket == SC_SOCKET_NONE);

    sc_mutex_lock(&intr->mutex);
    bool interrupted = intr->interrupted;
    if (!interrupted) {
        intr->process = pid;
    }
    sc_mutex_unlock(&intr->mutex);

    return !interrupted;
}

void
sc_intr_interrupt(struct sc_intr *intr) {
    atomic_store(&intr->interrupted, true);

    sc_mutex_lock(&intr->mutex);

    // No more than one component to interrupt
    assert(intr->socket == SC_SOCKET_NONE ||
           intr->process == SC_PROCESS_NONE);

    if (intr->socket != SC_SOCKET_NONE) {
        LOGD("Interrupting socket");
        net_interrupt(intr->socket);
        intr->socket = SC_SOCKET_NONE;
    }
    if (intr->process != SC_PROCESS_NONE) {
        LOGD("Interrupting process");
        sc_process_terminate(intr->process);
        intr->process = SC_PROCESS_NONE;
    }

    sc_mutex_unlock(&intr->mutex);
}

void
sc_intr_destroy(struct sc_intr *intr) {
    assert(intr->socket == SC_SOCKET_NONE);
    assert(intr->process == SC_PROCESS_NONE);

    sc_mutex_destroy(&intr->mutex);
}
