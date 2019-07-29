/**/

#include "thread.h"

int thr_create(thr_t *thread, int detached, thr_startfunc_ptr_t startfunc, void *arg)
{
    DWORD  ThreadID;
    HANDLE hThread = CreateThread(NULL, 0, startfunc, arg, 0, &ThreadID);

    if (hThread == NULL)
        return GetLastError();
    *thread = hThread;
    return 0;
}

int thr_cancel(thr_t thread)
{
    BOOL res = TerminateThread(thread, 0);
    if (!res)
        return GetLastError();
    return 0;
}

void thr_exit(thr_exitcode_t retval)
{
    ExitThread(retval);
}

thr_t thr_self(void)
{
    return GetCurrentThread();
}

int thr_equal(thr_t thread1, thr_t thread2)
{
    return thread1 == thread2;
}

int thr_join(thr_t thread, thr_exitcode_t *retval)
{
    return -1;
}

int thr_detach(thr_t thread)
{
    return 0;
}

thr_mutex_t thr_mutex_create(void)
{
    return CreateMutex(NULL, FALSE, NULL);
}

int thr_mutex_destroy(thr_mutex_t *mutex)
{
    return CloseHandle(*mutex) ? 0 : -1;
}

int thr_mutex_lock(thr_mutex_t *mutex)
{
    return WaitForSingleObject(*mutex, INFINITE) == WAIT_TIMEOUT ? -1 : 0;
}

int thr_mutex_trylock(thr_mutex_t *mutex)
{
    return WaitForSingleObject(*mutex, 0) == WAIT_TIMEOUT ? -1 : 0;
}

int thr_mutex_unlock(thr_mutex_t *mutex)
{
    return ReleaseMutex(*mutex) ? 0 : -1;
}

/**/
