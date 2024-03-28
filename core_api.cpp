/* 046267 Computer Architecture - HW #4 */

#include <vector>
#include "core_api.h"
#include "sim_api.h"

class Thread;

/* ----- globals ----- */

const int SWITCH = 8;

std::vector<Thread> g_fg_threads;
size_t g_fg_cycles = 0;
size_t g_fg_retire_count = 0;

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

    uint32_t get_pc() const
    {
        return m_pc;
    }

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

    bool is_finished() const
    {
        return m_finished;
    }

    void extract_context(tcontext * dest) const
    {
        for (int i = 0; i < REGS_COUNT; ++i)
        {
            dest->reg[i] = m_context.reg[i];
        }
    }
};

/* ----- Helper Functions ----- */

/**
 * @brief Perform a single cycle of the machine. This includes idling on all
 * threads waiting for memory operations, as well as executing a single
 * instruction in (at most) one active thread. The active thread executing an
 * instruction is picked using a RR.
 *
 * @param thread_count  Total number of threads in the core.
 * @param active_thread_count   Number of active threads in the core.
 * @return Number of active threads in the core at the end of the cycle. This
 * value can be lowered from the input number of active threads, reduced by one
 * if the thread that has executed reached a HALT instruction.
 */
int fg_perform_cycle(int thread_count, int active_thread_count)
{
    int tid = 0;
    int last_tid = 0;
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

        if (thread.is_finished())
        {
            --active_thread_count;
        }

        ++g_fg_retire_count;
    }

    ++g_fg_cycles;
    return active_thread_count;
}

/* ----- External API Functions ----- */

void CORE_BlockedMT()
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
}

void CORE_FinegrainedMT()
{
    int thread_count = SIM_GetThreadsNum();
    int active_thread_count = thread_count;

    g_fg_threads.assign(thread_count, Thread(SIM_GetLoadLat(),
                                             SIM_GetStoreLat()));

    while (active_thread_count > 0)
    {
        active_thread_count = fg_perform_cycle(thread_count,
                                               active_thread_count);
    }
}

double CORE_BlockedMT_CPI()
{
    return 0;
}

double CORE_FinegrainedMT_CPI()
{
    return (double)g_fg_cycles / (double)g_fg_retire_count;
}

void CORE_BlockedMT_CTX(tcontext * context, int threadid)
{

}

void CORE_FinegrainedMT_CTX(tcontext * context, int threadid)
{
    g_fg_threads.at(threadid).extract_context(&context[threadid]);
}
