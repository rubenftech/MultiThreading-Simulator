/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

const int SWITCH = 8;

// Arithmetic operations
void ADD(tcontext* context, int dst, int src1, int src2) {
    context->reg[dst] = context->reg[src1] + context->reg[src2];
}

void ADDI(tcontext* context, int dst, int src1, int imm) {
    context->reg[dst] = context->reg[src1] + imm;
}

void SUB(tcontext* context, int dst, int src1, int src2) {
    context->reg[dst] = context->reg[src1] - context->reg[src2];
}

void SUBI(tcontext* context, int dst, int src1, int imm) {
    context->reg[dst] = context->reg[src1] - imm;
}


// Find next thread to execute
static int FindNextThread(int currentThread, int totalThreads, const long int *remainingExecutionTime) {
    int nextThread = (currentThread + 1) % totalThreads;
    for (int i = 0; i < totalThreads; i++) {
        if (remainingExecutionTime[nextThread] != 0) {
            return nextThread;
        }
        nextThread = (nextThread + 1) % totalThreads;
    }
    return -1;
}

// Execute instruction and update context (not including load/store)
void ExecuteInstruction(Instruction instruction, tcontext *context) {
    switch (instruction.opcode) {
        case CMD_ADD:
            ADD(context, instruction.dst_index, instruction.src1_index, instruction.src2_index_imm);
            break;
        case CMD_ADDI:
            ADDI(context, instruction.dst_index, instruction.src1_index, instruction.src2_index_imm);
            break;
        case CMD_SUB:
            SUB(context, instruction.dst_index, instruction.src1_index, instruction.src2_index_imm);
            break;
        case CMD_SUBI:
            SUBI(context, instruction.dst_index, instruction.src1_index, instruction.src2_index_imm);
            break;
        default:
            break;
    }
}

// Perform load or store operation
static void PerformLoadOrStore(Instruction instruction, tcontext *threadContext) {
    int src2 = (instruction.isSrc2Imm) ? instruction.src2_index_imm : threadContext->reg[instruction.src2_index_imm];

    if (instruction.opcode == CMD_LOAD) {
        SIM_MemDataRead(threadContext->reg[instruction.src1_index] + src2, &threadContext->reg[instruction.dst_index]);
    } else if (instruction.opcode == CMD_STORE) {
        SIM_MemDataWrite(threadContext->reg[instruction.dst_index] + src2, threadContext->reg[instruction.src1_index]);
    }
}


// Update remaining execution time for all threads to simulate time passing
void UpdateThreadExecutionTime(int totalThreads, long int *remainingExecutionTime, int time) {
    for (int i = 0; i < totalThreads; i++) {
        if (remainingExecutionTime[i] != 0) {
            remainingExecutionTime[i] -= time;
        }
    }
}



void CORE_BlockedMT() {
    int completedThreadsCount = 0;
    int currentThreadIndex = 0;
    int blockedCycle = 0;
    int blockedOperation = 0;
    int totalThreads = SIM_GetThreadsNum();

    // Allocate register files
    tcontext *threadContexts = (tcontext *)malloc(totalThreads * sizeof(tcontext));
    // Init thread registers
    for (int threadIndex = 0; threadIndex < totalThreads; threadIndex++) {
        for (int regIndex = 0; regIndex < REGS_COUNT; regIndex++) {
            threadContexts[threadIndex].reg[regIndex] = 0;
        }
    }
    // Allocate remaining execution time array
    int commandExecutionTime[] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
    commandExecutionTime[CMD_LOAD] += SIM_GetLoadLat();
    commandExecutionTime[CMD_STORE] += SIM_GetStoreLat();
    commandExecutionTime[SWITCH] = SIM_GetSwitchCycles();

    long int *remainingExecutionTime = (long int *)calloc(totalThreads, sizeof(long int));
    uint32_t *nextInstructionLine = (uint32_t *)calloc(totalThreads, sizeof(uint32_t));

    while (completedThreadsCount != totalThreads) {
        // Find next thread to execute
        int nextThreadIndex = FindNextThread(currentThreadIndex, totalThreads, remainingExecutionTime);
        // FILL IN CODE HERE
    }
}

void CORE_FinegrainedMT() {

}

double CORE_BlockedMT_CPI() {

}

double CORE_FinegrainedMT_CPI(){

}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {

}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {

}



