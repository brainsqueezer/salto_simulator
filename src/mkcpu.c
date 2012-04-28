

/**
 * microcode word format
 *
 * 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * [  rsel (5)  ] [ aluf(4) ] [ bs(3)] [  f1(4)  ] [  f2(4)  ] LT LL  [        next(10)          ]
 *
 *
 *                 0  0  0  0   ALU <- BUS
 *                              PROM: 1111/1/0; function F=A; T source is ALU
 *
 *                 0  0  0  1   ALU <- T
 *                              PROM: 1010/1/0; function F=B; T source is BUS
 *
 *                 0  0  1  0   ALU <- BUS | T
 *                              PROM: 1110/1/0; function F=A|B; T source is ALU
 *
 *                 0  0  1  1   ALU <- BUS & T
 *                              PROM: 1011/1/0; function F=A&B; T source is BUS
 *
 *                 0  1  0  0   ALU <- BUS ^ T
 *                              PROM: 0110/1/0; function F=A^B; T source is BUS
 *
 *                 0  1  0  1   ALU <- BUS + 1
 *                              PROM: 0000/0/0; function F=A+1; T source is ALU
 *
 *                 0  1  1  0   ALU <- BUS - 1
 *                              PROM: 1111/0/1; function F=A-1; T source is ALU
 *
 *                 0  1  1  1   ALU <- BUS + T
 *                              PROM: 1001/0/1; function F=A+B; T source is BUS
 *
 *                 1  0  0  0   ALU <- BUS - T
 *                              PROM: 0110/0/0; function F=A-B; T source is BUS
 *
 *                 1  0  0  1   ALU <- BUS - T - 1
 *                              PROM: 0110/0/1; function F=A-B-1; T source is BUS
 *
 *                 1  0  1  0   ALU <- BUS + T + 1
 *                              PROM: 1001/0/0; function F=A+B+1; T source is ALU
 *
 *                 1  0  1  1   ALU <- BUS + SKIP
 *                              PROM: 0000/0/SKIP; function F=A+(~SKIP); T source is ALU
 *
 *                 1  1  0  0   ALU <- BUS & T
 *                              PROM: 1011/1/0; function F=A&B; T source is ALU
 *
 *                 1  1  0  1   ALU <- BUS & ~T
 *                              PROM: 0111/1/0; function F=A&~B; T source is BUS
 *
 *                 1  1  1  0   ALU <- ???
 *                              PROM: ????/?/?; perhaps F=0 (0011/0/0); T source is BUS
 *
 *                 1  1  1  1   ALU <- ???
 *                              PROM: ????/?/?; perhaps F=0 (0011/0/0); T source is BUS
 *
 *                             0  0  0   BUS source is R register
 *
 *                             0  0  1   load R register from BUS
 *
 *                             0  1  0   BUS is open (0177777)
 *
 *                             0  1  1   BUS source is task specific
 *
 *
 *                             1  0  0   BUS source is task specific
 *
 *                             1  0  1   BUS source is memory data
 *
 *                             1  1  0   BUS source is mouse data
 *
 *                             1  1  1   BUS source displacement
 *
 */
