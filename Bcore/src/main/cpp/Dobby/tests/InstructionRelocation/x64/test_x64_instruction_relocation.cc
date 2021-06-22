#define CATCH_CONFIG_MAIN

#include "catch.hpp"

#include <string.h>

#include "InstructionRelocation/x64/X64IPRelativeOpcodeTable.h"

/*

sub_140006C30        48 89 5C 24 08                                   mov     [rsp+arg_0], rbx
sub_140006C30+5      48 89 74 24 10                                   mov     [rsp+arg_8], rsi
sub_140006C30+A      57                                               push    rdi
sub_140006C30+B      48 83 EC 40                                      sub     rsp, 40h
sub_140006C30+F      48 8B F9                                         mov     rdi, rcx
sub_140006C30+12     48 8D 99 70 02 00 00                             lea     rbx, [rcx+270h]
sub_140006C30+19     8B CA                                            mov     ecx, edx
sub_140006C30+1B     8B F2                                            mov     esi, edx
sub_140006C30+1D     E8 0E CA FF FF                                   call    sub_140003660
sub_140006C30+1D
sub_140006C30+22     48 8B 0F                                         mov     rcx, [rdi]
sub_140006C30+25     48 8D 15 24 17 07 00                             lea     rdx, aCDvsP4BuildSwG_1      ;
"C:\\dvs\\p4\\build\\sw\\gcomp\\dev\\src"... sub_140006C30+2C     48 89 5C 24 30                                   mov
[rsp+48h+var_18], rbx sub_140006C30+31     41 B9 03 00 00 00                                mov     r9d, 3
sub_140006C30+37     48 89 44 24 28                                   mov     [rsp+48h+var_20], rax
sub_140006C30+3C     41 B8 F9 00 00 00                                mov     r8d, 0F9h
sub_140006C30+42     48 8D 05 EF 19 07 00                             lea     rax, aInitiateTransi        ; "Initiate
transition to %s state for %s" sub_140006C30+49     48 89 44 24 20                                   mov
[rsp+48h+var_28], rax sub_140006C30+4E     E8 ED D5 00 00                                   call    sub_140014270
sub_140006C30+4E
sub_140006C30+53     48 83 BF D0 02 00 00 00                          cmp     qword ptr [rdi+2D0h], 0
sub_140006C30+5B     74 10                                            jz      short loc_140006C9D
sub_140006C30+5B
sub_140006C30+5D     E8 7E CA 00 00                                   call    sub_140013710
sub_140006C30+5D
sub_140006C30+62     48 8B 8F D0 02 00 00                             mov     rcx, [rdi+2D0h]
sub_140006C30+69     48 89 41 48                                      mov     [rcx+48h], rax
sub_140006C30+69
sub_140006C30+6D
sub_140006C30+6D                                                  loc_140006C9D:                          ; CODE XREF:
sub_140006C30+5B↑j sub_140006C30+6D     8B 97 34 03 00 00                                mov     edx, [rdi+334h]
sub_140006C30+73     48 8B 4F 08                                      mov     rcx, [rdi+8]
sub_140006C30+77     89 B7 B0 02 00 00                                mov     [rdi+2B0h], esi
sub_140006C30+7D     48 8B 5C 24 50                                   mov     rbx, [rsp+48h+arg_0]
sub_140006C30+82     48 8B 74 24 58                                   mov     rsi, [rsp+48h+arg_8]
sub_140006C30+87     48 83 C4 40                                      add     rsp, 40h
sub_140006C30+8B     5F                                               pop     rdi
sub_140006C30+8C     E9 2F BC FF FF                                   jmp     sub_1400028F0

*/

//------------------------------------------------------------
//-----------       Created with 010 Editor        -----------
//------         www.sweetscape.com/010editor/          ------
//
// File    : C:\Users\jmpews\Downloads\NvContainer\nvcontainer.exe
// Address : 24624 (0x6030)
// Size    : 145 (0x91)
//------------------------------------------------------------
unsigned char hexData[145] = {
    0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x40, 0x48, 0x8B, 0xF9, 0x48,
    0x8D, 0x99, 0x70, 0x02, 0x00, 0x00, 0x8B, 0xCA, 0x8B, 0xF2, 0xE8, 0x0E, 0xCA, 0xFF, 0xFF, 0x48, 0x8B, 0x0F, 0x48,
    0x8D, 0x15, 0x24, 0x17, 0x07, 0x00, 0x48, 0x89, 0x5C, 0x24, 0x30, 0x41, 0xB9, 0x03, 0x00, 0x00, 0x00, 0x48, 0x89,
    0x44, 0x24, 0x28, 0x41, 0xB8, 0xF9, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0xEF, 0x19, 0x07, 0x00, 0x48, 0x89, 0x44,
    0x24, 0x20, 0xE8, 0xED, 0xD5, 0x00, 0x00, 0x48, 0x83, 0xBF, 0xD0, 0x02, 0x00, 0x00, 0x00, 0x74, 0x10, 0xE8, 0x7E,
    0xCA, 0x00, 0x00, 0x48, 0x8B, 0x8F, 0xD0, 0x02, 0x00, 0x00, 0x48, 0x89, 0x41, 0x48, 0x8B, 0x97, 0x34, 0x03, 0x00,
    0x00, 0x48, 0x8B, 0x4F, 0x08, 0x89, 0xB7, 0xB0, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x5C, 0x24, 0x50, 0x48, 0x8B, 0x74,
    0x24, 0x58, 0x48, 0x83, 0xC4, 0x40, 0x5F, 0xE9, 0x2F, 0xBC, 0xFF, 0xFF};

// clang-format off
int instrLenArray[] = {
  5,
  5,
  1,
  4,
  3,
  7,
  2,
  2,
  5,
  3,
  7,
  5,
  6,
  5,
  6,
  7,
  5,
  5,
  8,
  2,
  5,
  7,
  4,
  6,
  4,
  6,
  5,
  5,
  4,
  1,
  5
};
// clang-format on

TEST_CASE(">>> InstructionRelocation/x64", "[InstructionRelocation]") {
  void *TargetFunction = hexData;
  uintptr_t srcIP = (uintptr_t)TargetFunction;
  uintptr_t currIP = srcIP;
  int funcLen = sizeof(hexData);
  unsigned char opcode1 = 0;
  InstrMnemonic instr = {0};

  int i = 0;
  opcode1 = *(byte *)srcIP;

  do {
    OpcodeDecodeItem *decodeItem = &OpcodeDecodeTable[opcode1];
    decodeItem->DecodeHandler(&instr, (addr_t)currIP);

    REQUIRE(instr.len == instrLenArray[i]);
    currIP += instr.len;
    opcode1 = *(byte *)currIP;
    if (instr.instr.opcode) {
      printf("ndx %d: %d\n", i, instr.len);
      // clear instr
      memset((void *)&instr, 0, sizeof(InstrMnemonic));
    }
    i++;
  } while (currIP < (srcIP + funcLen));

  printf("InstructionRelocation/x64 Done!");
}
