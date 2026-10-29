--
-- lua-Harness : <https://fperrad.frama.io/lua-Harness/>
--
-- Copyright (C) 2014-2025, Perrad Francois
--
-- This code is licensed under the terms of the MIT/X11 license,
-- like Lua itself.
--

do -- offset
    array_equals({utf8.offset("A\u{20AC}3", 1)}, {1, 1}, "function offset")
    array_equals({utf8.offset("A\u{20AC}3", 2)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 3)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", 4)}, {6, 6})
    array_equals({utf8.offset("A\u{20AC}3", 5)}, {})
    array_equals({utf8.offset("A\u{20AC}3", 6)}, {})
    array_equals({utf8.offset("A\u{20AC}3", -1)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", 1, 2)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 2, 2)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", 3, 2)}, {6, 6})
    array_equals({utf8.offset("A\u{20AC}3", 4, 2)}, {})
    array_equals({utf8.offset("A\u{20AC}3", -1, 2)}, {1, 1})
    array_equals({utf8.offset("A\u{20AC}3", -2, 2)}, {})
    array_equals({utf8.offset("A\u{20AC}3", 1, 5)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", 2, 5)}, {6, 6})
    array_equals({utf8.offset("A\u{20AC}3", 3, 5)}, {})
    array_equals({utf8.offset("A\u{20AC}3", -1, 5)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", -2, 5)}, {1, 1})
    array_equals({utf8.offset("A\u{20AC}3", -3, 5)}, {})
    array_equals({utf8.offset("A\u{20AC}3", 1, 6)}, {6, 6})
    array_equals({utf8.offset("A\u{20AC}3", 2, 6)}, {})
    array_equals({utf8.offset("A\u{20AC}3", 1, -1)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", -1, -1)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", -2, -1)}, {1, 1})
    array_equals({utf8.offset("A\u{20AC}3", -3, -1)}, {})
    array_equals({utf8.offset("A\u{20AC}3", 1, -4)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 2, -4)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", -1, -4)}, {1, 1})
    array_equals({utf8.offset("A\u{20AC}3", -2, -4)}, {})

    array_equals({utf8.offset("A\u{20AC}3", 0, 1)}, {1, 1})
    array_equals({utf8.offset("A\u{20AC}3", 0, 2)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 0, 3)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 0, 4)}, {2, 4})
    array_equals({utf8.offset("A\u{20AC}3", 0, 5)}, {5, 5})
    array_equals({utf8.offset("A\u{20AC}3", 0, 6)}, {6, 6})
end

-- Local Variables:
--   mode: lua
--   lua-indent-level: 4
--   fill-column: 100
-- End:
-- vim: ft=lua expandtab shiftwidth=4:
