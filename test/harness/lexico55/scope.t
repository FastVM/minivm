#! /usr/bin/lua
--
-- lua-Harness : <https://fperrad.frama.io/lua-Harness/>
--
-- Copyright (C) 2009-2025, Perrad Francois
--
-- This code is licensed under the terms of the MIT/X11 license,
-- like Lua itself.
--

--[[ scope ]]
global x, equals
x = 10
do
    local x = x
    equals(x, 10, "scope")
    x = x + 1
    do
        local x = x + 1
        equals(x, 12)
    end
    equals(x, 11)
end
equals(x, 10)

-- Local Variables:
--   mode: lua
--   lua-indent-level: 4
--   fill-column: 100
-- End:
-- vim: ft=lua expandtab shiftwidth=4:
