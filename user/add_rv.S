    .data
brackets_fmt:  .asciz "|%s|\n"
result_fmt:    .asciz "%d\n"
err_fmt:       .asciz "add: invalid input format\n"

#define BUFFER_SIZE 512
#define ASCII_SPACE 32
#define ASCII_MINUS 45
#define ASCII_LF 10

    .text   
    .globl main

main:
    addi sp, sp, -BUFFER_SIZE
    mv    s0, sp

    # Read line using gets(s0)
    mv    a0, s0
    call  gets

    mv    t0, s0
remove_newline:
    lbu   t1, 0(t0)
    beq   t1, zero, newline_done   # If end of string reached then exit
    li    t2, ASCII_LF
    beq   t1, t2, newline_replace
    addi  t0, t0, 1
    j     remove_newline
newline_replace:
    sb    zero, 0(t0)
newline_done:

    # Print string using format |%s|\n
    la    a0, brackets_fmt
    mv    a1, s0
    call  printf

    # Find first whitespace
    mv    t0, s0
find_space:
    lbu   t1, 0(t0)
    beq   t1, zero, invalid_format  # If end of string then error
    li    t2, ASCII_SPACE
    beq   t1, t2, found_space
    addi  t0, t0, 1
    j     find_space

found_space:
    sb    zero, 0(t0)
    addi  t3, t0, 1

    # Первая подстрока – начало буфера
    mv    t4, s0            # t4 = first_ptr

    lbu   t1, 0(t4)
    beq   t1, zero, invalid_format

    # Sign handling for first number (t5 = first_sign)
    li    t5, 1
    lbu   t1, 0(t4)
    li    t2, ASCII_MINUS
    bne   t1, t2, first_no_sign
    li    t5, -1
    addi  t4, t4, 1
first_no_sign:
    lbu   t1, 0(t4)
    beq   t1, zero, invalid_format

    # Sign handling for second number (t6 = second_sign)
    li    t6, 1
    lbu   t1, 0(t3)
    li    t2, ASCII_MINUS
    bne   t1, t2, second_no_sign
    li    t6, -1
    addi  t3, t3, 1
second_no_sign:
    lbu   t1, 0(t3)
    beq   t1, zero, invalid_format

    # First substring into number: atoi()
    mv    a0, t4
    call  atoi
    mul   a0, a0, t5
    mv    t1, a0

    # Second substring into number: atoi()
    mv    a0, t3
    call  atoi
    mul   a0, a0, t6
    mv    t2, a0

    # Do syscall add(a, b)
    mv    a0, t1
    mv    a1, t2
    call  add

    # Save result into s1
    mv    s1, a0

    # Printing result: printf("%d\n", sum)
    la    a0, result_fmt
    mv    a1, s1
    call  printf

    li    a0, 0
    call  exit

invalid_format:
    la    a0, err_fmt
    call  printf
    li    a0, 1
    call  exit