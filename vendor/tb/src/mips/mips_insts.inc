// special
R(sllv, 0b000000, 0b000100)
R(srlv, 0b000000, 0b000110)
R(srav, 0b000000, 0b000111)
R(sll,  0b000000, 0b000000)
R(srl,  0b000000, 0b000010)
R(sra,  0b000000, 0b000011)
R(add,  0b000000, 0b100000)
R(addu, 0b000000, 0b100001)
R(sub,  0b000000, 0b100010)
R(subu, 0b000000, 0b100011)
R(and,  0b000000, 0b100100)
R(or,   0b000000, 0b100101)
R(xor,  0b000000, 0b100110)
R(nor,  0b000000, 0b100111)
R(jr,   0b000000, 0b001000)
// special2
R(mul,  0b011100, 0b000010)
// i-types
I(addi, 0b001000)
I(andi, 0b001100)
I(ori,  0b001101)
I(xori, 0b001110)
I(lui,  0b001111)
// loads
I(lb,   0b100000)
I(lw,   0b100011)
I(ld,   0b110111)
// stores
I(sb,   0b101000)
I(sh,   0b101001)
I(sw,   0b101011)
// branches
I(beq,  0b000100)
J(jal,  0b000011)
#undef R
#undef I
#undef J