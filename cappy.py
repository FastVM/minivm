
from capstone import *

with open("out1.bin", "rb") as f:
    CODE = f.read()

md = Cs(CS_ARCH_X86, CS_MODE_64)
max_len = max(len(i.mnemonic) for i in md.disasm(CODE, 0x1000))
str = '\n'.join(str(i.op_str) for i in md.disasm(CODE, 0x1000))
with open("out.asm", "w") as f:
    for k, asm in enumerate(md.disasm(CODE, 0x1000)):
        if k == 0 or hex(asm.address) in str:
            f.write(f"0x{asm.address:x}:\n")
        f.write(f"  {asm.mnemonic}{' ' * (max_len + 1 - len(asm.mnemonic))}{asm.op_str}\n")
