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
    } else if (instruction.opcode == CMD_STORE){
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
    return 0;

}

double CORE_FinegrainedMT_CPI(){
    return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {

}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {

}


//////////////////////////////////////////// TESTS////////////////////////////////////////////
void test_ADD() {
    tcontext context;
    // Initialize registers
    context.reg[0] = 5;
    context.reg[1] = 10;
    // Execute ADD
    ADD(&context, 2, 0, 1); // Expected: reg[2] = 15
    // Check the result
    assert(context.reg[2] == 15);

    // Additional test case
    context.reg[0] = 20;
    context.reg[1] = 30;
    ADD(&context, 2, 0, 1); // Expected: reg[2] = 50
    assert(context.reg[2] == 50);
}

void test_ADDI() {
    tcontext context;
    context.reg[0] = 5;
    // Execute ADDI
    ADDI(&context, 1, 0, 3); // Expected: reg[1] = 8
    assert(context.reg[1] == 8);

    // Additional test case
    ADDI(&context, 1, 0, -3); // Expected: reg[1] = 2
    assert(context.reg[1] == 2);
}

void test_SUB() {
    tcontext context;
    context.reg[0] = 15;
    context.reg[1] = 5;
    // Execute SUB
    SUB(&context, 2, 0, 1); // Expected: reg[2] = 10
    assert(context.reg[2] == 10);

    // Additional test case
    SUB(&context, 2, 1, 0); // Expected: reg[2] = -10
    assert(context.reg[2] == -10);
}

void test_SUBI() {
    tcontext context;
    context.reg[0] = 10;
    // Execute SUBI
    SUBI(&context, 1, 0, 5); // Expected: reg[1] = 5
    assert(context.reg[1] == 5);

    // Additional test case
    SUBI(&context, 1, 0, -5); // Expected: reg[1] = 15
    assert(context.reg[1] == 15);
}

void test_FindNextThread() {
    long int remainingExecutionTime[] = {0, 10, 5, 0};
    int totalThreads = sizeof(remainingExecutionTime) / sizeof(long int);
    // Execute FindNextThread
    int nextThread = FindNextThread(0, totalThreads, remainingExecutionTime);
    // Expected: next thread is 1
    assert(nextThread == 1);

    // Additional test case: No active threads
    remainingExecutionTime[1] = 0;
    remainingExecutionTime[2] = 0;
    nextThread = FindNextThread(0, totalThreads, remainingExecutionTime);
    // Expected: no active thread, return -1
    assert(nextThread == -1);
}

void test_ExecuteInstruction_ADD() {
    tcontext context;
    Instruction instruction = {CMD_ADD, 2, 0, 1, false};
    context.reg[0] = 5;
    context.reg[1] = 10;
    ExecuteInstruction(instruction, &context);
    assert(context.reg[2] == 15);
}

void test_UpdateThreadExecutionTime() {
    long int remainingExecutionTime[] = {10, 20, 0, 5};
    int totalThreads = sizeof(remainingExecutionTime) / sizeof(long int);
    UpdateThreadExecutionTime(totalThreads, remainingExecutionTime, 5);
    assert(remainingExecutionTime[0] == 5);
    assert(remainingExecutionTime[1] == 15);
    assert(remainingExecutionTime[2] == 0);
    assert(remainingExecutionTime[3] == 0);
}

void test_PerformLoad() {
    // Initialize simulated memory

    tcontext context;
    context.reg[0] = 0;  // Test value for src1
    context.reg[1] = 10; // Base address for loading
    context.reg[2] = 0;  // Initial value of the destination register

    Instruction instruction = {CMD_LOAD, 2, 1, 0, false}; // LOAD instruction

    PerformLoadOrStore(instruction, &context);

    assert(context.reg[2] == 123); // Check if the loaded value is correct
}

void test_PerformStore() {
    // Initialize simulated memory

    tcontext context;
    context.reg[0] = 0;  // Test value for src1
    context.reg[1] = 456; // Value to store
    context.reg[2] = 10;  // Base address for storage

    Instruction instruction = {CMD_STORE, 1, 2, 0, false}; // STORE instruction

    PerformLoadOrStore(instruction, &context);

    // Check if the value is correctly stored
    //assert(simulatedMemory[10] == 456); // This assumes simulatedMemory is accessible here

    // Additional test case: storing at a different address
    context.reg[2] = 20; // New base address for storage
    PerformLoadOrStore(instruction, &context);
    //assert(simulatedMemory[20] == 456); // Check the new storage location
}
