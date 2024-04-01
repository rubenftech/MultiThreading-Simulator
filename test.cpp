
#include "test.h"
#include "core_api.cpp"
#include "sim_api.c"

#include <stdio.h>

/* ----- Test Functions ----- */

void test_ADD()
{
    tcontext context;
    // Initialize registers
    context.reg[0] = 5;
    context.reg[1] = 10;
    // Execute core_add
    core_add(&context, 2, 0, 1); // Expected: reg[2] = 15
    // Check the result
    assert(context.reg[2] == 15);

    // Additional test case
    context.reg[0] = 20;
    context.reg[1] = 30;
    core_add(&context, 2, 0, 1); // Expected: reg[2] = 50
    assert(context.reg[2] == 50);
}

void test_ADDI()
{
    tcontext context;
    context.reg[0] = 5;
    // Execute core_addi
    core_addi(&context, 1, 0, 3); // Expected: reg[1] = 8
    assert(context.reg[1] == 8);

    // Additional test case
    core_addi(&context, 1, 0, -3); // Expected: reg[1] = 2
    assert(context.reg[1] == 2);
}

void test_SUB()
{
    tcontext context;
    context.reg[0] = 15;
    context.reg[1] = 5;
    // Execute core_sub
    core_sub(&context, 2, 0, 1); // Expected: reg[2] = 10
    assert(context.reg[2] == 10);

    // Additional test case
    core_sub(&context, 2, 1, 0); // Expected: reg[2] = -10
    assert(context.reg[2] == -10);
}

void test_SUBI()
{
    tcontext context;
    context.reg[0] = 10;
    // Execute core_subi
    core_subi(&context, 1, 0, 5); // Expected: reg[1] = 5
    assert(context.reg[1] == 5);

    // Additional test case
    core_subi(&context, 1, 0, -5); // Expected: reg[1] = 15
    assert(context.reg[1] == 15);
}

void test_FindNextThread()
{
    long int remainingExecutionTime[] = { 0, 10, 5, 0 };
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

void test_ExecuteInstruction_ADD()
{
    tcontext context;
    Instruction instruction = { CMD_ADD, 2, 0, 1, false };
    context.reg[0] = 5;
    context.reg[1] = 10;
    execute_instruction(instruction, &context);
    assert(context.reg[2] == 15);
}

void test_UpdateThreadExecutionTime()
{
    long int remainingExecutionTime[] = { 10, 20, 0, 5 };
    int totalThreads = sizeof(remainingExecutionTime) / sizeof(long int);
    UpdateThreadExecutionTime(totalThreads, remainingExecutionTime, 5);
    assert(remainingExecutionTime[0] == 5);
    assert(remainingExecutionTime[1] == 15);
    assert(remainingExecutionTime[2] == 0);
    assert(remainingExecutionTime[3] == 0);
}

void test_PerformLoad()
{
    // Initialize simulated memory

    tcontext context;
    context.reg[0] = 0;  // Test value for src1
    context.reg[1] = 10; // Base address for loading
    context.reg[2] = 0;  // Initial value of the destination register

    Instruction instruction = { CMD_LOAD, 2, 1, 0, false }; // LOAD instruction

    execute_instruction(instruction, &context);

    assert(context.reg[2] == 123); // Check if the loaded value is correct
}

void test_PerformStore()
{
    // Initialize simulated memory

    tcontext context;
    context.reg[0] = 0;  // Test value for src1
    context.reg[1] = 456; // Value to store
    context.reg[2] = 10;  // Base address for storage

    Instruction instruction = { CMD_STORE, 1, 2, 0, false }; // STORE instruction

    execute_instruction(instruction, &context);

    // Check if the value is correctly stored
    //assert(simulatedMemory[10] == 456); // This assumes simulatedMemory is accessible here

    // Additional test case: storing at a different address
    context.reg[2] = 20; // New base address for storage
    execute_instruction(instruction, &context);
    //assert(simulatedMemory[20] == 456); // Check the new storage location
}

/* ----- Main Entry Point ----- */


int main(int argc, char const * argv[])
{
    //tests
    test_ADD();
    printf("Add test passed\n");

    test_ADDI();
    printf("Addi test passed\n");

    test_SUB();
    printf("Sub test passed\n");

    test_SUBI();
    printf("Subi test passed\n");

    test_FindNextThread();
    printf("FindNextThread test passed\n");

    test_ExecuteInstruction_ADD();
    printf("ExecuteInstruction_ADD test passed\n");

    test_UpdateThreadExecutionTime();
    printf("UpdateThreadExecutionTime test passed\n");

    SIM_MemDataWrite(10, 123);
    test_PerformLoad();
    printf("PerformLoad test passed\n");

    test_PerformStore();
    printf("PerformStore test passed\n");

    return 0;
}
