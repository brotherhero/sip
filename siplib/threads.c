/*
 * Thread support for the SIP library.  This module provides the hooks for
 * C++ classes that provide a thread interface to interact properly with the
 * Python threading infrastructure.
 *
 * Copyright (c) 2008
 * 	Phil Thompson <phil@river-bank.demon.co.uk>
 * 
 * This file is part of SIP.
 * 
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 * 
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#include "sip.h"
#include "sipint.h"


/*
 * The data associated with pending request to wrap an object.
 */
typedef struct _pendingDef {
    void *cpp;                      /* The C/C++ object ot be wrapped. */
    sipWrapper *owner;              /* The owner of the object. */
    int flags;                      /* The flags. */
} pendingDef;


#ifdef WITH_THREAD

#include <pythread.h>


/*
 * The per thread data we need to maintain.
 */
typedef struct _threadDef {
    long thr_ident;                 /* The thread identifier. */
    pendingDef pending;             /* An object waiting to be wrapped. */
    struct _threadDef *next;        /* Next in the list. */
} threadDef;


static threadDef *threads = NULL;   /* Linked list of threads. */


static threadDef *currentThreadDef(void);

#endif


static pendingDef pending;          /* An object waiting to be wrapped. */


/*
 * Get the address of any C/C++ object waiting to be wrapped.
 */
void *sipGetPending(sipWrapper **op, int *fp)
{
    pendingDef *pp;

#ifdef WITH_THREAD
    threadDef *td;

    if ((td = currentThreadDef()) != NULL)
        pp = &td->pending;
    else
        pp = &pending;
#else
    pp = &pending;
#endif

    if (pp->cpp != NULL)
    {
        if (op != NULL)
            *op = pp->owner;

        if (fp != NULL)
            *fp = pp->flags;
    }

    return pp->cpp;
}


/*
 * Convert a new C/C++ pointer to a Python instance.
 */
PyObject *sipWrapSimpleInstance(void *cppPtr, sipWrapperType *type,
        sipWrapper *owner, int flags)
{
    static PyObject *nullargs = NULL;

    pendingDef old_pending;
    PyObject *self;
#ifdef WITH_THREAD
    threadDef *td;
#endif

    if (nullargs == NULL && (nullargs = PyTuple_New(0)) == NULL)
        return NULL;

    if (cppPtr == NULL)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    /*
     * Object creation can trigger the Python garbage collector which in turn
     * can execute arbitrary Python code which can then call this function
     * recursively.  Therefore we save any existing pending object before
     * setting the new one.
     */
#ifdef WITH_THREAD
    if ((td = currentThreadDef()) != NULL)
    {
        old_pending = td->pending;

        td->pending.cpp = cppPtr;
        td->pending.owner = owner;
        td->pending.flags = flags;
    }
    else
    {
        old_pending = pending;

        pending.cpp = cppPtr;
        pending.owner = owner;
        pending.flags = flags;
    }
#else
    old_pending = pending;

    pending.cpp = cppPtr;
    pending.owner = owner;
    pending.flags = flags;
#endif

    self = PyObject_Call((PyObject *)type, nullargs, NULL);

#ifdef WITH_THREAD
    if (td != NULL)
        td->pending = old_pending;
    else
        pending = old_pending;
#else
    pending = old_pending;
#endif

    return self;
}


/*
 * This is called from a newly created thread to initialise some thread local
 * storage.
 */
void sip_api_start_thread(void)
{
#ifdef WITH_THREAD
    threadDef *td;

    /* Save the thread ID.  First, find an empty slot in the list. */
    for (td = threads; td != NULL; td = td->next)
        if (td->thr_ident == 0)
            break;

    if (td == NULL)
    {
        td = sip_api_malloc(sizeof (threadDef));
        td->next = threads;
        threads = td;
    }

    if (td != NULL)
    {
        td->thr_ident = PyThread_get_thread_ident();
        td->pending.cpp = NULL;
    }
#endif
}


/*
 * Handle the termination of a thread.  The thread state should already have
 * been handled by the last call to PyGILState_Release().
 */
void sip_api_end_thread(void)
{
#ifdef WITH_THREAD
    threadDef *td;

    /* We have the GIL at this point. */
    if ((td = currentThreadDef()) != NULL)
        td->thr_ident = 0;
#endif
}


#ifdef WITH_THREAD

/*
 * Return the thread data for the current thread or NULL if it wasn't
 * recognised.
 */
static threadDef *currentThreadDef(void)
{
    threadDef *td;
    long ident = PyThread_get_thread_ident();

    for (td = threads; td != NULL; td = td->next)
        if (td->thr_ident == ident)
            break;

    return td;
}

#endif