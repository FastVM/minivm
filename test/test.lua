
std.io = {}
std.io.putchar = std.extern("vm_std_io_putchar")
std.io.debug = std.extern("vm_std_io_debug")
std.io.debug(std)
