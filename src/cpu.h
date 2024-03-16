#ifndef CPU_H
#define CPU_H

#include "common.h"
#include "ppu.h"
#include "apu.h"

uint32 P;
uint32 A;
uint32 X;
uint32 Y;
uint32 S;
uint32 PC;

#define P_C (1 << 0)
#define P_Z (1 << 1)
#define P_I (1 << 2)
#define P_D (1 << 3)
#define P_B (1 << 4)
#define P_U (1 << 5)
#define P_V (1 << 6)
#define P_N (1 << 7)

#define SET_C(V)\
    P &= ~P_C;\
    if (V) P |= P_C

#define SET_ZN(V)\
    P &= ~(P_Z | P_N);\
    if (V == 0)\
        P |= P_Z;\
    else if (V & 0x80)\
        P |= P_N

#define SET_CZN(V)\
    P &= ~(P_C | P_Z | P_N);\
    if (V == 0)\
        P |= P_Z;\
    else if (V & 0x80)\
        P |= P_N;\
    if (V >= 0)\
        P |= P_C

#define READ(ADDR)      cpu_read(ADDR)
#define WRITE(ADDR, V)  cpu_write(ADDR, V);

#define FETCH()         READ(PC++)

#define MODE_IMM_ADDR() (PC++)
#define MODE_ABS_ADDR() (FETCH() | (FETCH() << 8))
#define MODE_ABX_ADDR() ((MODE_ABS_ADDR() + X) & 0xFFFF)
#define MODE_ABY_ADDR() ((MODE_ABS_ADDR() + Y) & 0xFFFF)
#define MODE_REL_ADDR() ((sint8)FETCH())
#define MODE_ZP0_ADDR() FETCH()
#define MODE_ZPX_ADDR() ((MODE_ZP0_ADDR() + X) & 0xFF)
#define MODE_ZPY_ADDR() ((MODE_ZP0_ADDR() + Y) & 0xFF)
#define MODE_IND_ADDR() ind_impl()
#define MODE_IZX_ADDR() izx_impl()
#define MODE_IZY_ADDR() izy_impl()

#define MODE_IMP() A
#define MODE_IMM() READ(MODE_IMM_ADDR())
#define MODE_ABS() READ(MODE_ABS_ADDR())
#define MODE_ABX() READ(MODE_ABX_ADDR())
#define MODE_ABY() READ(MODE_ABY_ADDR())
#define MODE_REL() MODE_REL_ADDR()
#define MODE_ZP0() READ(MODE_ZP0_ADDR())
#define MODE_ZPX() READ(MODE_ZPX_ADDR())
#define MODE_ZPY() READ(MODE_ZPY_ADDR())
#define MODE_IND() READ(MODE_IND_ADDR())
#define MODE_IZX() READ(MODE_IZX_ADDR())
#define MODE_IZY() READ(MODE_IZY_ADDR())

uint32 ind_impl(void)
{
    uint32 addr = MODE_ABS_ADDR();
    if ((addr & 0xFF) == 0xFF)
        return READ(addr) | (READ(addr & 0xFF00) << 8);
    return READ(addr) | (READ(addr + 1) << 8);
}

uint32 izx_impl(void)
{
    uint32 t = FETCH() + X;
    uint32 l = READ(t & 0xFF);
    uint32 h = READ((t + 1) & 0xFF);
    return l | (h << 8);
}

uint32 izy_impl(void)
{
    uint32 t = FETCH();
    uint32 l = READ(t & 0xFF);
    uint32 h = READ((t + 1) & 0xFF);
    return ((l | (h << 8)) + Y) & 0xFFFF;
}

#define PUSH_8(V)   WRITE(0x0100 + (S--), V)
#define PUSH_16(V)  PUSH_8((V) >> 8); PUSH_8(V)
#define POP_8()     READ(0x0100 + (++S))
//#define POP_16()    (POP_8() | (POP_8() << 8)) // WTF?

uint32 POP_16(void)
{
    uint32 l = POP_8();
    uint32 h = POP_8();
    return l | (h << 8);
}

#define LD(REG, MODE) REG = MODE(); SET_ZN(REG)
#define ST(REG, MODE) WRITE(MODE##_ADDR(), REG)

#define OP_ADC(MODE)\
    uint32 v = MODE();\
    uint32 t = v + A + (P & P_C);\
    P &= ~(P_C | P_Z | P_V | P_N);\
    if (t & 0xFF00) P |= P_C;\
    if (!(t & 0xFF)) P |= P_Z;\
    if ((~(A ^ v) & (A ^ t)) & 0x80) P |= P_V;\
    if (t & 0x80) P |= P_N;\
    A = t & 0xFF

#define OP_AND(MODE) A &= MODE(); SET_ZN(A)

// ASL for A
#define OP_ASA(MODE)\
    uint32 t = (A << 1);\
    A = t & 0xFF;\
    SET_C(t & 0xFF00);\
    SET_ZN(A);

#define OP_ASL(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 t = READ(addr) << 1;\
    SET_C(t & 0xFF00);\
    t &= 0xFF;\
    SET_ZN(t);\
    WRITE(addr, t);

#define OP_BCC(MODE) sint32 t = MODE(); if (!(P & P_C)) PC += t
#define OP_BCS(MODE) sint32 t = MODE(); if (P & P_C) PC += t
#define OP_BEQ(MODE) sint32 t = MODE(); if (P & P_Z) PC += t

#define OP_BIT(MODE)\
    uint32 t = MODE();\
    P &= ~(P_Z | P_V | P_N);\
    if (!(t & A & 0x00FF)) P |= P_Z;\
    if (t & 0x80) P |= P_N;\
    if (t & 0x40) P |= P_V

#define OP_BMI(MODE) sint32 t = MODE(); if (P & P_N) PC += t
#define OP_BNE(MODE) sint32 t = MODE(); if (!(P & P_Z)) PC += t
#define OP_BPL(MODE) sint32 t = MODE(); if (!(P & P_N)) PC += t
#define OP_BRK(MODE) TODO
#define OP_BVC(MODE) sint32 t = MODE(); if (!(P & P_V)) PC += t
#define OP_BVS(MODE) sint32 t = MODE(); if (P & P_V) PC += t
#define OP_CLC(MODE) P &= ~P_C
#define OP_CLD(MODE) P &= ~P_D
#define OP_CLI(MODE) P &= ~P_I
#define OP_CLV(MODE) P &= ~P_V
#define OP_CMP(MODE) sint32 t = A - MODE(); SET_CZN(t)
#define OP_CPX(MODE) sint32 t = X - MODE(); SET_CZN(t)
#define OP_CPY(MODE) sint32 t = Y - MODE(); SET_CZN(t)

#define OP_DEC(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 t = READ(addr) - 1;\
    WRITE(addr, t);\
    SET_ZN(t);

#define OP_DEX(MODE) X = (X - 1) & 0xFF; SET_ZN(X)
#define OP_DEY(MODE) Y = (Y - 1) & 0xFF; SET_ZN(Y)
#define OP_EOR(MODE) A ^= MODE(); SET_ZN(A)

#define OP_INC(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 t = (READ(addr) + 1) & 0xFF;\
    WRITE(addr, t);\
    SET_ZN(t)

#define OP_INX(MODE) X = (X + 1) & 0xFF; SET_ZN(X)
#define OP_INY(MODE) Y = (Y + 1) & 0xFF; SET_ZN(Y)
#define OP_JMP(MODE) PC = MODE##_ADDR()
#define OP_JSR(MODE) uint32 addr = MODE##_ADDR(); PC--; PUSH_16(PC); PC = addr
#define OP_LDA(MODE) LD(A, MODE)
#define OP_LDX(MODE) LD(X, MODE)
#define OP_LDY(MODE) LD(Y, MODE)
#define OP_LSA(MODE) SET_C(A & 1); A >>= 1; SET_ZN(A) // LSR for A

#define OP_LSR(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 t = READ(addr);\
    SET_C(t & 1);\
    t >>= 1;\
    SET_ZN(t);\
    WRITE(addr, t)

#define OP_NOP(MODE) // nothing to do
#define OP_ORA(MODE) A |= MODE(); SET_ZN(A)
#define OP_PHA(MODE) PUSH_8(A)
#define OP_PHP(MODE) PUSH_8(P | P_B | P_U); P &= ~(P_B | P_U)
#define OP_PLA(MODE) A = POP_8(); SET_ZN(A)
#define OP_PLP(MODE) P = POP_8(); P |= P_U

// ROL for A
#define OP_RAL(MODE)\
    uint32 t = (A << 1) | (P & P_C);\
    SET_ZN(t);\
    SET_C(t & 0xFF00);\
    A = t & 0xFF

#define OP_ROL(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 t = READ(addr);\
    t = (t << 1) | (P & P_C);\
    SET_ZN(t);\
    SET_C(t & 0xFF00);\
    WRITE(addr, t)

#define OP_RAR(MODE)\
    uint32 t = (A >> 1) | ((P & P_C) << 7);\
    SET_ZN(t);\
    SET_C(A & 0x01);\
    A = t & 0xFF

#define OP_ROR(MODE)\
    uint32 addr = MODE##_ADDR();\
    uint32 f = READ(addr);\
    uint32 t = (f >> 1) | ((P & P_C) << 7);\
    SET_ZN(t);\
    SET_C(f & 0x01);\
    WRITE(addr, t)

#define OP_RTI(MODE) P = POP_8() & ~(P_B | P_U); PC = POP_16()
#define OP_RTS(MODE) PC = POP_16() + 1

#define OP_SBC(MODE)\
    uint32 v = MODE() ^ 0x00FF;\
    uint32 t = A + v + (P & P_C);\
    P &= ~(P_C | P_Z | P_V | P_N);\
    if (t & 0xFF00) P |= P_C;\
    if (!(t & 0x00FF)) P |= P_Z;\
    if ((t ^ A) & (t ^ v) & 0x80) P |= P_V;\
    if (t & 0x80) P |= P_N;\
    A = t & 0xFF;

#define OP_SEC(MODE) P |= P_C
#define OP_SED(MODE) P |= P_D
#define OP_SEI(MODE) P |= P_I
#define OP_STA(MODE) ST(A, MODE)
#define OP_STX(MODE) ST(X, MODE)
#define OP_STY(MODE) ST(Y, MODE)
#define OP_TAX(MODE) X = A; SET_ZN(X)
#define OP_TAY(MODE) Y = A; SET_ZN(Y)
#define OP_TSX(MODE) X = S; SET_ZN(X)
#define OP_TXA(MODE) A = X; SET_ZN(A)
#define OP_TXS(MODE) S = X
#define OP_TYA(MODE) A = Y; SET_ZN(A)

#define DECL_U(u) NULL,
#define IMPL_U(u)

#define DECL(n, m) n##_##m,
//#define IMPL(n, m) void n##_##m(void) { LOG("%04X %s (%s)", PC - 1, #n, #m); OP_##n(MODE_##m); }
#define IMPL(n, m) void n##_##m(void) { OP_##n(MODE_##m); }

#define OP_TABLE(E,U)\
    E(BRK,IMP) E(ORA,IZX) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ORA,ZP0) E(ASL,ZP0) U(UNKNOWN) E(PHP,IMP) E(ORA,IMM) E(ASA,IMP) U(UNKNOWN) U(UNKNOWN) E(ORA,ABS) E(ASL,ABS) U(UNKNOWN)\
    E(BPL,REL) E(ORA,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ORA,ZPX) E(ASL,ZPX) U(UNKNOWN) E(CLC,IMP) E(ORA,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ORA,ABX) E(ASL,ABX) U(UNKNOWN)\
    E(JSR,ABS) E(AND,IZX) U(UNKNOWN) U(UNKNOWN) E(BIT,ZP0) E(AND,ZP0) E(ROL,ZP0) U(UNKNOWN) E(PLP,IMP) E(AND,IMM) E(RAL,IMP) U(UNKNOWN) E(BIT,ABS) E(AND,ABS) E(ROL,ABS) U(UNKNOWN)\
    E(BMI,REL) E(AND,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(AND,ZPX) E(ROL,ZPX) U(UNKNOWN) E(SEC,IMP) E(AND,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(AND,ABX) E(ROL,ABX) U(UNKNOWN)\
    E(RTI,IMP) E(EOR,IZX) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(EOR,ZP0) E(LSR,ZP0) U(UNKNOWN) E(PHA,IMP) E(EOR,IMM) E(LSA,IMP) U(UNKNOWN) E(JMP,ABS) E(EOR,ABS) E(LSR,ABS) U(UNKNOWN)\
    E(BVC,REL) E(EOR,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(EOR,ZPX) E(LSR,ZPX) U(UNKNOWN) E(CLI,IMP) E(EOR,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(EOR,ABX) E(LSR,ABX) U(UNKNOWN)\
    E(RTS,IMP) E(ADC,IZX) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ADC,ZP0) E(ROR,ZP0) U(UNKNOWN) E(PLA,IMP) E(ADC,IMM) E(RAR,IMP) U(UNKNOWN) E(JMP,IND) E(ADC,ABS) E(ROR,ABS) U(UNKNOWN)\
    E(BVS,REL) E(ADC,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ADC,ZPX) E(ROR,ZPX) U(UNKNOWN) E(SEI,IMP) E(ADC,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(ADC,ABX) E(ROR,ABX) U(UNKNOWN)\
    U(UNKNOWN) E(STA,IZX) U(UNKNOWN) U(UNKNOWN) E(STY,ZP0) E(STA,ZP0) E(STX,ZP0) U(UNKNOWN) E(DEY,IMP) U(UNKNOWN) E(TXA,IMP) U(UNKNOWN) E(STY,ABS) E(STA,ABS) E(STX,ABS) U(UNKNOWN)\
    E(BCC,REL) E(STA,IZY) U(UNKNOWN) U(UNKNOWN) E(STY,ZPX) E(STA,ZPX) E(STX,ZPY) U(UNKNOWN) E(TYA,IMP) E(STA,ABY) E(TXS,IMP) U(UNKNOWN) U(UNKNOWN) E(STA,ABX) U(UNKNOWN) U(UNKNOWN)\
    E(LDY,IMM) E(LDA,IZX) E(LDX,IMM) U(UNKNOWN) E(LDY,ZP0) E(LDA,ZP0) E(LDX,ZP0) U(UNKNOWN) E(TAY,IMP) E(LDA,IMM) E(TAX,IMP) U(UNKNOWN) E(LDY,ABS) E(LDA,ABS) E(LDX,ABS) U(UNKNOWN)\
    E(BCS,REL) E(LDA,IZY) U(UNKNOWN) U(UNKNOWN) E(LDY,ZPX) E(LDA,ZPX) E(LDX,ZPY) U(UNKNOWN) E(CLV,IMP) E(LDA,ABY) E(TSX,IMP) U(UNKNOWN) E(LDY,ABX) E(LDA,ABX) E(LDX,ABY) U(UNKNOWN)\
    E(CPY,IMM) E(CMP,IZX) U(UNKNOWN) U(UNKNOWN) E(CPY,ZP0) E(CMP,ZP0) E(DEC,ZP0) U(UNKNOWN) E(INY,IMP) E(CMP,IMM) E(DEX,IMP) U(UNKNOWN) E(CPY,ABS) E(CMP,ABS) E(DEC,ABS) U(UNKNOWN)\
    E(BNE,REL) E(CMP,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(CMP,ZPX) E(DEC,ZPX) U(UNKNOWN) E(CLD,IMP) E(CMP,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(CMP,ABX) E(DEC,ABX) U(UNKNOWN)\
    E(CPX,IMM) E(SBC,IZX) U(UNKNOWN) U(UNKNOWN) E(CPX,ZP0) E(SBC,ZP0) E(INC,ZP0) U(UNKNOWN) E(INX,IMP) E(SBC,IMM) E(NOP,IMP) E(SBC,IMP) E(CPX,ABS) E(SBC,ABS) E(INC,ABS) U(UNKNOWN)\
    E(BEQ,REL) E(SBC,IZY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(SBC,ZPX) E(INC,ZPX) U(UNKNOWN) E(SED,IMP) E(SBC,ABY) U(UNKNOWN) U(UNKNOWN) U(UNKNOWN) E(SBC,ABX) E(INC,ABX) U(UNKNOWN)

typedef void (*op_func)(void);

OP_TABLE(IMPL, IMPL_U)

static const op_func op_table[] = { OP_TABLE(DECL, DECL_U) };

void cpu_reset(void)
{
    P = P_U | P_B;
    A = 0;
    X = 0;
    Y = 0;
    S = 0xFD;
    PC = 0xFFFC;
    PC = MODE_ABS_ADDR();

    memset(ram, 0, sizeof(ram));
}

uint32 map_addr(uint32 addr)
{
    return addr & ((prg_banks > 1) ? 0x7FFF : 0x3FFF);
}

uint32 cpu_read(uint32 addr)
{
    ASSERT(addr <= 0xFFFF);

    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        return prg_rom[map_addr(addr)];
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        addr &= 0x07FF;
        return ram[addr];
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        addr &= 0x0007;
        return ppu_read(addr);
    }
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        addr &= 0x1F;
        return apu_read(addr);
    }
    else
    {
        //ASSERT(0);
    }
    return 0;
}

void cpu_write(uint32 addr, uint8 data)
{
    if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        prg_rom[map_addr(addr)] = data;
    }
    else if (addr >= 0x0000 && addr <= 0x1FFF)
    {
        addr &= 0x07FF;
        ram[addr] = data;
    }
    else if (addr >= 0x2000 && addr <= 0x3FFF)
    {
        addr &= 0x0007;
        ppu_write(addr, data);
    }
    else if (addr >= 0x4000 && addr <= 0x4017)
    {
        addr &= 0x1F;
        apu_write(addr, data);
    }
    else
    {
        ASSERT(0);
    }
}

void cpu_nmi(void)
{
    PUSH_16(PC);
    P = (P | P_I | P_U) & ~P_B;
    PUSH_8(P);

    PC = 0xFFFA;
    PC = MODE_ABS_ADDR();
}

void cpu_clock(sint32 cycles)
{
    while (cycles--)
    {
        uint32 op = FETCH();
        ASSERT(op_table[op]);
        op_table[op]();
    }
}

#endif
