#include "kernel/types.h"
#include "user/user.h"

#define HEAP_SIZE 4096 * 4
#define STACK_SIZE 1024

void print_testname(const char *message) {
    printf("\n\n----- %s -----\n", message);
}

int global_var;
void test_global_var() {
    print_testname("Global Variable");
    printf("\n=== Initial state ===\n");
    vmprint(0, 0, 0);

    global_var = 100;
    printf("\n=== After global variable write ===\n");
    vmprint((const char *)&global_var, 1, 0);

    vmclear(0, 0, 3);
    printf("\n=== After flags clear ===\n");
    vmprint(0, 0, 0);

    int x = global_var;
    printf("\n=== After global variable read ===\n");
    vmprint((const char *)&global_var, 1, 0);

    global_var = x;
    vmclear(0, 0, 3);
}

void test_stack_var() {
    print_testname("Stack Variable");
    printf("\n=== Initial state ===\n");
    vmprint(0, 0, 0);

    int stack_var;
    printf("\n=== After allocating ===\n");
    vmprint(0, 0, 0);

    stack_var = 100;
    printf("\n=== After stack variable write ===\n");
    vmprint((const char *)&stack_var, 1, 0);

    vmclear(0, 0, 3);
    printf("\n=== After flags clear ===\n");
    vmprint(0, 0, 0);

    global_var = stack_var;
    printf("\n=== After stack variable read ===\n");
    vmprint((const char *)&stack_var, 1, 0);

    vmclear(0, 0, 3);
}

void test_stack_array() {
    print_testname("Stack Array");
    printf("\n=== Initial state ===\n");
    vmprint(0, 0, 0);

    char stack[STACK_SIZE];
    printf("\n=== After allocation ===\n");
    vmprint(0, 0, 0);

    stack[1] = 42;
    printf("\n=== After stack write ===\n");
    vmprint(stack, sizeof(stack), 0);

    vmclear(0, 0, 3);
    printf("\n=== After flags clear ===\n");
    vmprint(0, 0, 0);

    global_var = stack[STACK_SIZE-1];
    printf("\n=== After stack read ===\n");
    vmprint(stack, sizeof(stack), 0);

    vmclear(0, 0, 3);
}

void test_heap_array() {
    print_testname("Heap Array");
    printf("\n=== Initial state ===\n");
    vmprint(0, 0, 0);

    char *heap = malloc(HEAP_SIZE);
    printf("\n=== After allocation ===\n");
    vmprint(0, 0, 0);

    heap[1] = 42;
    printf("\n=== After heap write ===\n");
    vmprint(heap, HEAP_SIZE, 0);

    vmclear(0, 0, 3);
    printf("\n=== After flags clear ===\n");
    vmprint(0, 0, 0);

    global_var = heap[4096];
    printf("\n=== After heap read ===\n");
    vmprint(heap, HEAP_SIZE, 0);

    vmclear(0, 0, 3);
    printf("\n=== After flags clear ===\n");
    vmprint(0, 0, 0);

    free(heap);
    printf("\n=== After memory free ===\n");
    vmprint(0, 0, 0);

    vmclear(0, 0, 3);
}

int main() {
    test_global_var();
    test_stack_var();
    test_stack_array();
    test_heap_array();
    exit(0);
}