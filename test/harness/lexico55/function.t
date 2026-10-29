#! /usr/bin/lua
--
-- lua-Harness : <https://fperrad.frama.io/lua-Harness/>
--
-- Copyright (C) 2009-2025, Perrad Francois
--
-- This code is licensed under the terms of the MIT/X11 license,
-- like Lua itself.
--

do --[[ var args ]]
    local function g1(a, b, ...t)
        equals(a, 3, "vararg")
        equals(b, nil)
        equals(t.n, 0)
        equals(t[1], nil)
    end
    g1(3)

    local function g2(a, b, ...t)
        equals(a, 3)
        equals(b, 4)
        equals(t.n, 0)
        equals(t[1], nil)
    end
    g2(3, 4)

    local function g3(a, b, ...t)
        equals(a, 3)
        equals(b, 4)
        equals(t.n, 2)
        equals(t[1], 5)
        equals(t[2], 8)
    end
    g3(3, 4, 5, 8)
end

-- Local Variables:
--   mode: lua
--   lua-indent-level: 4
--   fill-column: 100
-- End:
-- vim: ft=lua expandtab shiftwidth=4:
