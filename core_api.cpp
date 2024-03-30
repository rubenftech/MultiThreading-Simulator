/* 046267 Computer Architecture - HW #4 */

#include <vector>
#include "core_api.h"
#include "sim_api.h"

class Thread;

/* ----- globals ----- */

//const int SWITCH = 8;

std::vector<Thread> g_fg_threads;
size_t g_fg_cycles = 0;
size_t g_fg_retire_count = 0;

std::vector<Thread> g_b_threads;
size_t g_b_cycles = 0;
size_t g_b_retire_count = 0;

/* ----- Arithmetic Operations ----- */

void core_add(tcontext * context, int dst, int src1, int src2)
{
    context->reg[dst] = context->reg[src1] + context->reg[src2];
}

void core_addi(tcontext * context, int dst, int src1, int imm)
{
    context->reg[dst] = context->reg[src1] + imm;
}

void core_sub(tcontext * context, int dst, int src1, int src2)
{
    context->reg[dst] = context->reg[src1] - context->reg[src2];
}

void core_subi(tcontext * context, int dst, int src1, int imm)
{
    context->reg[dst] = context->reg[src1] - imm;
}

/* ----- Multithreading Functions ----- */

// Find next thread to execute
int
FindNextThread(
    int currentThread,
    int totalThreads,
    const long int * remainingExecutionTime)
{
    int nextThread = (currentThread + 1) % totalThreads;
    for (int i = 0; i < totalThreads; i++)
    {
        if (remainingExecutionTime[nextThread] != 0)
        {
            return nextThread;
        }
        nextThread = (nextThread + 1) % totalThreads;
    }
    return -1;
}

// Execute instruction and update context (not including load/store)
void
execute_instruction(
    Instruction instruction,
    tcontext * context)
{
    int src2;

    switch (instruction.opcode)
    {
        case CMD_ADD:
            core_add(context,
                     instruction.dst_index,
                     instruction.src1_index,
                     instruction.src2_index_imm);
            break;
        case CMD_ADDI:
            core_addi(context,
                      instruction.dst_index,
                      instruction.src1_index,
                      instruction.src2_index_imm);
            break;
        case CMD_SUB:
            core_sub(context,
                     instruction.dst_index,
                     instruction.src1_index,
                     instruction.src2_index_imm);
            break;
        case CMD_SUBI:
            core_subi(context,
                      instruction.dst_index,
                      instruction.src1_index,
                      instruction.src2_index_imm);
            break;
        case CMD_LOAD:
            src2 = instruction.isSrc2Imm ?
                   instruction.src2_index_imm :
                   context->reg[instruction.src2_index_imm];
            SIM_MemDataRead(context->reg[instruction.src1_index] + src2,
                            &context->reg[instruction.dst_index]);
            break;
        case CMD_STORE:
            src2 = instruction.isSrc2Imm ?
                   instruction.src2_index_imm :
                   context->reg[instruction.src2_index_imm];
            SIM_MemDataWrite(context->reg[instruction.dst_index] + src2,
                             context->reg[instruction.src1_index]);
            break;
        default:
            break;
    }
}

// Update remaining execution time for all threads to simulate time passing
void
UpdateThreadExecutionTime(
    int totalThreads,
    long int * remainingExecutionTime,
    int time)
{
    for (int i = 0; i < totalThreads; i++)
    {
        if (remainingExecutionTime[i] != 0)
        {
            remainingExecutionTime[i] -= time;
        }
    }
}

/* ----- Classes ----- */

class Thread
{
private:
    tcontext m_context;
    size_t m_pc;

    size_t m_load_latency;
    size_t m_store_latency;
    size_t m_latency_count;

    bool m_finished;

public:
    Thread(size_t load_latency, size_t store_latency) :
        m_context{ 0 },
        m_pc(0),
        m_load_latency(load_latency),
        m_store_latency(store_latency),
        m_latency_count(0),
        m_finished(false)
    {}

    /**
     * @brief Get the current PC of the thread.
     */
    size_t get_pc() const
    {
        return m_pc;
    }

    /**
     * @brief Execute the given instruction.
     * @param instruction Instruction to execute.
     * @return `true` if an instruction completed successfully, `false`
     * in case the thread is inactive.
     */
    bool execute(Instruction instruction)
    {
        int src2;
        if (m_latency_count > 0)
        {
            return false;
        }

        switch (instruction.opcode)
        {
            case CMD_ADD:
                core_add(&m_context,
                         instruction.dst_index,
                         instruction.src1_index,
                         instruction.src2_index_imm);
                break;
            case CMD_ADDI:
                core_addi(&m_context,
                          instruction.dst_index,
                          instruction.src1_index,
                          instruction.src2_index_imm);
                break;
            case CMD_SUB:
                core_sub(&m_context,
                         instruction.dst_index,
                         instruction.src1_index,
                         instruction.src2_index_imm);
                break;
            case CMD_SUBI:
                core_subi(&m_context,
                          instruction.dst_index,
                          instruction.src1_index,
                          instruction.src2_index_imm);
                break;
            case CMD_LOAD:
                src2 = instruction.isSrc2Imm ?
                       instruction.src2_index_imm :
                       m_context.reg[instruction.src2_index_imm];
                SIM_MemDataRead(m_context.reg[instruction.src1_index] + src2,
                                &m_context.reg[instruction.dst_index]);
                m_latency_count += m_load_latency;
                break;
            case CMD_STORE:
                src2 = instruction.isSrc2Imm ?
                       instruction.src2_index_imm :
                       m_context.reg[instruction.src2_index_imm];
                SIM_MemDataWrite(m_context.reg[instruction.dst_index] + src2,
                                 m_context.reg[instruction.src1_index]);
                m_latency_count += m_store_latency;
                break;
            case CMD_HALT:
                m_finished = true;
                break;
            case CMD_NOP:
                break;
        }

        ++m_pc;
        return true;
    }

    /**
     * @brief Perform an idle cycle. This is a cycle where the thread does not
     * execute a command. This could be because it is finished, waiting for
     * data from memory, or another thread is active.
     * @note This is an _active_ method - the thread's state might change. Do
     * not call this method more than once a cycle, or in the same cycle as the
     * `execute` method (on the same thread).
     * @return `true` if the thread is inactive (finished/waiting for memory),
     * `false` otherwise.
     */
    bool idle()
    {
        if (m_finished)
        {
            return true;
        }

        if (m_latency_count > 0)
        {
            --m_latency_count;
            return true;
        }

        return false;
    }

    /**
     * @brief Check if the thread finished execution (occurs if the thread
     * reached a HALT command).
     */
    bool is_finished() const
    {
        return m_finished;
    }

    /**
     * @brief Copy the current context to the given container.
     * @param dest Empty context to copy values to.
     */
    void extract_context(tcontext * dest) const
    {
        memcpy(dest, &m_context, sizeof(m_context));
    }
};

/* ----- Helper Functions ----- */

/**
 * @brief Perform a single cycle of the machine in fine-grained mode. This
 * includes idling on all threads waiting for memory operations, as well as
 * executing a single instruction in (at most) one active thread. The active
 * thread executing an instruction is picked using a RR.
 *
 * @param thread_count IN   Total number of threads in the core.
 * @param active_thread_count IN    Number of active threads in the core.
 * @param last_tid INOUT    Last tid that was picked by the RR for execution.
 * If a thread executed this cycle, `last_tid` updates to its id.
 * @return Number of active threads in the core at the end of the cycle. This
 * value can be lowered from the input number of active threads, reduced by one
 * if the thread that has executed reached a HALT instruction.
 */
int fg_perform_cycle(int thread_count, int active_thread_count, int &last_tid)
{
    int tid = 0;
    int picked_tid = -1;
    bool is_picked = false;
    Instruction instruction;

    for (int i = 0; i < thread_count; ++i)
    {
        tid = (i + last_tid) % thread_count;
        Thread &thread = g_fg_threads.at(tid);

        if (!thread.idle() && !is_picked)
        {
            picked_tid = tid;
            is_picked = true;
        }
    }

    if (is_picked)
    {
        Thread &thread = g_fg_threads.at(picked_tid);
        SIM_MemInstRead(thread.get_pc(), &instruction, picked_tid);
        thread.execute(instruction);

        // If the thread finished, remove it from active count.
        if (thread.is_finished())
        {
            --active_thread_count;
        }

        // Increment count of executed instructions.
        ++g_fg_retire_count;

        // Update last tid.
        last_tid = picked_tid;
    }

    ++g_fg_cycles;
    return active_thread_count;
}

/**
 * @brief Perform a single cycle of the machine in blocking mode. This includes
 * idling on all threads waiting for memory operations, as well as executing a
 * single instruction in (at most) one active thread. The active thread
 * executing an instruction is picked using a RR.
 *
 * Whenever switching between threads, some cycles of penalty are taken where
 * the machine cannot execute any instructions.
 *
 * @param thread_count IN   Total number of threads in the core.
 * @param active_thread_count IN    Number of active threads in the core.
 * @param context_switch_penalty IN Number of cycles the cpu cannot execute
 * instructions whenever it is context-switching.
 * @param last_tid INOUT    Last tid that was picked by the RR for execution.
 * If a thread executed this cycle, `last_tid` updates to its id.
 * @return Number of active threads in the core at the end of the cycle. This
 * value can be lowered from the input number of active threads, reduced by one
 * if the thread that has executed reached a HALT instruction.
 */
int b_perform_cycle(
    int thread_count,
    int active_thread_count,
    int context_switch_penalty,
    int &last_tid)
{
    int tid = 0;
    int picked_tid = -1;
    bool is_picked = false;
    Instruction instruction;

    // Start from 1 to skip the last thread, which we tested separately.
    for (int i = 0; i < thread_count; ++i)
    {
        tid = (i + last_tid) % thread_count;
        Thread &thread = g_b_threads.at(tid);

        if (!thread.idle() && !is_picked)
        {
            // If the picked thread is different from the last active one, do a
            // context switch (during which all threads are idle).
            if (tid != last_tid)
            {
                for (int j = 0; j < context_switch_penalty; ++j)
                {
                    for (int k = 0; k < thread_count; ++k)
                    {
                        g_b_threads.at(k).idle();
                    }
                    ++g_b_cycles;
                }
            }
            picked_tid = tid;
            is_picked = true;
        }
    }

    if (is_picked)
    {
        Thread &thread = g_b_threads.at(picked_tid);
        SIM_MemInstRead(thread.get_pc(), &instruction, picked_tid);
        thread.execute(instruction);

        // If the thread finished, remove it from active count.
        if (thread.is_finished())
        {
            --active_thread_count;
        }

        // Increment count of executed instructions.
        ++g_b_retire_count;

        // Update last tid.
        last_tid = picked_tid;
    }

    ++g_b_cycles;
    return active_thread_count;
}

/* ----- External API Functions ----- */

/*void CORE_BlockedMT()
{
    int completedThreadsCount = 0;
//    int currentThreadIndex = 0;
//    int blockedCycle = 0;
//    int blockedOperation = 0;
    int totalThreads = SIM_GetThreadsNum();

    // Allocate register files
    tcontext * threadContexts = (tcontext *)malloc(
        totalThreads * sizeof(tcontext));
    // Init thread registers
    for (int threadIndex = 0; threadIndex < totalThreads; threadIndex++)
    {
        for (int regIndex = 0; regIndex < REGS_COUNT; regIndex++)
        {
            threadContexts[threadIndex].reg[regIndex] = 0;
        }
    }

    // Allocate remaining execution time array
    int commandExecutionTime[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    commandExecutionTime[CMD_LOAD] += SIM_GetLoadLat();
    commandExecutionTime[CMD_STORE] += SIM_GetStoreLat();
    commandExecutionTime[SWITCH] = SIM_GetSwitchCycles();

    long int * remainingExecutionTime = (long int *)calloc(totalThreads,
                                                           sizeof(long int));
    uint32_t * nextInstructionLine = (uint32_t *)calloc(totalThreads,
                                                        sizeof(uint32_t));

    while (completedThreadsCount != totalThreads)
    {
        // Find next thread to execute
//        int nextThreadIndex = FindNextThread(currentThreadIndex, totalThreads,
//                                             remainingExecutionTime);
        // FILL IN CODE HERE
        break;
    }

    free(threadContexts);
    free(remainingExecutionTime);
    free(nextInstructionLine);
}*/

void CORE_BlockedMT()
{
    int thread_count = SIM_GetThreadsNum();
    int active_thread_count = thread_count;
    int context_switch_penalty = SIM_GetSwitchCycles();
    int last_tid = 0;

    g_b_threads.assign(thread_count, Thread(SIM_GetLoadLat(),
                                             SIM_GetStoreLat()));

    while (active_thread_count > 0)
    {
        active_thread_count = b_perform_cycle(thread_count,
                                              active_thread_count,
                                              context_switch_penalty,
                                              last_tid);
    }
}

void CORE_FinegrainedMT()
{
    int thread_count = SIM_GetThreadsNum();
    int active_thread_count = thread_count;
    int last_tid = 0;

    g_fg_threads.assign(thread_count, Thread(SIM_GetLoadLat(),
                                             SIM_GetStoreLat()));

    while (active_thread_count > 0)
    {
        active_thread_count = fg_perform_cycle(thread_count,
                                               active_thread_count,
                                               last_tid);
    }
}

double CORE_BlockedMT_CPI()
{
    return (double)g_b_cycles / (double)g_b_retire_count;
}

double CORE_FinegrainedMT_CPI()
{
    return (double)g_fg_cycles / (double)g_fg_retire_count;
}

void CORE_BlockedMT_CTX(tcontext * context, int threadid)
{
    g_b_threads.at(threadid).extract_context(&context[threadid]);
}

void CORE_FinegrainedMT_CTX(tcontext * context, int threadid)
{
    g_fg_threads.at(threadid).extract_context(&context[threadid]);
}
