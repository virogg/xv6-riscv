#include "kernel/types.h"
#include "user/user.h"

#define PTE_A (1 << 5)
#define PTE_D (1 << 6)
#define HEAP_SIZE (4 * 4096)
#define STACK_SIZE 1024

int main() {
    char *heap = malloc(HEAP_SIZE);
    int stack[STACK_SIZE];

    stack[0] = 0;
    printf("Stack init: %d\n", stack[0]);

    printf("=== Initial state ===\n");
    vmprint();

    heap[0] = 'A';
    printf("\n=== After heap write ===\n");
    vmprint();

    vmclear(PTE_A | PTE_D);
    printf("\n=== After flags clear ===\n");
    vmprint();

    printf("\nReading heap: %s\n", *heap[0]);
    printf("=== After heap read ===\n");
    vmprint();

    stack[0] = 42;
    printf("\nStack value: %d", stack[0]);
    printf("\n=== After stack write ===\n");
    vmprint();

    free(heap);
    printf("\n=== After heap free ===\n");
    vmprint();

    exit(0);
}