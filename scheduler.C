/*
 File: scheduler.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

static Scheduler *idle_scheduler = nullptr;

void Scheduler::idle_func()
{
    for (;;)
    {
        idle_scheduler->yield();
    }
}

/* Add to zombie queue helper */
void Scheduler::queue_zombie(Thread *_thread)
{
    if (!zombie_head)
    {
        zombie_head = _thread;
        zombie_tail = _thread;
    }
    else
    {
        zombie_tail->next = _thread;
        zombie_tail = _thread;
    }
    _thread->next = nullptr;
}

Scheduler::Scheduler()
{
    idle_scheduler = this;
    char *idle_stack = new char[1024];
    idle_thread = new Thread(idle_func, idle_stack, 1024);

    head = idle_thread;
    tail = idle_thread;
    current_thread = Thread::CurrentThread();

    zombie_head = nullptr;
    zombie_tail = nullptr;

    assert(true);
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
    /* Zombie Reaper */
    Machine::disable_interrupts();
    while (zombie_head)
    {
        Thread *next_zombie = zombie_head->next;
        zombie_head->next = nullptr;
        delete[] zombie_head->stack;
        delete zombie_head;
        zombie_head = next_zombie;
        Console::puts("Reaped zombie.\n");
    }
    zombie_tail = nullptr;

    if (Thread::CurrentThread() == head && !head->next) /* If idle thread is running and queue is empty (only idle thread) */
    {
        Machine::enable_interrupts();
        return;
    }
    else if (head == tail) /* Only idle thread in queue */
    {
        current_thread = idle_thread;
        Thread::dispatch_to(idle_thread);
    }
    else if (head->next == tail)
    {
        Thread *next_thread = head->next;

        next_thread->next = nullptr;
        next_thread->prev = nullptr;

        tail = head;
        head->next = nullptr;
        tail->prev = nullptr;

        if (next_thread == Thread::CurrentThread()) 
       {
            Machine::enable_interrupts();
            return;
        }

        current_thread = next_thread;
        Thread::dispatch_to(next_thread);
    }
    else
    {
        Thread *next_thread = head->next;
        head->next = next_thread->next;
        next_thread->next->prev = head;

        next_thread->next = nullptr;
        next_thread->prev = nullptr;

        current_thread = next_thread;
        Thread::dispatch_to(next_thread);
    }
    Machine::enable_interrupts();

    assert(true);
    Console::puts("Thread yielded.\n");
}

void Scheduler::resume(Thread *_thread)
{
    Machine::disable_interrupts();
    if (tail == head) /* if only idle thread is present */
    {
        tail = _thread;
        head->next = tail;
        head->prev = nullptr;
        tail->prev = head;
        tail->next = nullptr;
    }
    else
    {
        tail->next = _thread;
        _thread->prev = tail;
        tail = _thread;
        tail->next = nullptr;
    }
    Machine::enable_interrupts();

    assert(true);
    Console::puts("Thread resumed.\n");
}

void Scheduler::add(Thread *_thread)
{
    resume(_thread);
    assert(true);
    Console::puts("Thread added.\n");
}

void Scheduler::terminate(Thread *_thread)
{
    current_thread = Thread::CurrentThread();
    if (_thread == current_thread && !head->next)
    {
        _thread->next = nullptr;
        _thread->prev = nullptr;
        queue_zombie(_thread);
        current_thread = idle_thread;
        Thread::dispatch_to(idle_thread);
    }
    else if (_thread == current_thread && head->next == tail)
    {
        _thread->next = nullptr;
        _thread->prev = nullptr;
        Thread *next_thread = head->next;
        next_thread->next = nullptr;
        next_thread->prev = nullptr;
        head = idle_thread;
        tail = head;
        head->next = nullptr;
        tail->prev = nullptr;
        queue_zombie(_thread);
        current_thread = next_thread;
        Thread::dispatch_to(next_thread);
    }
    else if (_thread == current_thread)
    {
        _thread->next = nullptr;
        _thread->prev = nullptr;

        Thread *next_thread = head->next;
        next_thread->next->prev = next_thread->prev;
        next_thread->prev->next = next_thread->next;

        next_thread->next = nullptr;
        next_thread->prev = nullptr;
        queue_zombie(_thread);
        current_thread = next_thread;
        Thread::dispatch_to(next_thread);
    }
    else if (_thread == tail && _thread == head->next)
    {
        _thread->next = nullptr;
        _thread->prev = nullptr;
        tail = head;
        head->next = nullptr;
        tail->prev = nullptr;
        queue_zombie(_thread);
    }
    else if (_thread == tail)
    {
        tail = _thread->prev;
        tail->next = nullptr;
        _thread->next = nullptr;
        _thread->prev = nullptr;
        queue_zombie(_thread);
    }
    else
    {
        _thread->prev->next = _thread->next;
        _thread->next->prev = _thread->prev;
        _thread->next = nullptr;
        _thread->prev = nullptr;
        queue_zombie(_thread);
    }
    assert(true);
    Console::puts("Thread terminated.\n");
}
