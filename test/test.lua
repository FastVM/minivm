
local function decl(name, sym)
    rawset(std, name, std.extern(sym))
end

local function main()
    decl("putchar", "vm_std_io_putchar")
end

main()

std.putchar(10)
