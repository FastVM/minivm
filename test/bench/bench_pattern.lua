-- Spatial and Pattern Recognition Benchmark
package.cpath = 'build/cmod/?.so;' .. package.cpath
local pat = require('luapattern')

local N = 5000
local R = 15.0
local CELL = 16.0
local entities = {}

-- Deterministic pseudo-random generation
local seed = 123456789
local function rng()
    seed = (seed * 1103515245 + 12345) & 0x7fffffff
    return seed / 0x7fffffff
end

for i = 1, N do
    entities[i] = {
        id = i,
        x = rng() * 500.0,
        y = rng() * 500.0,
    }
end

-- 1. Naive O(N^2) Pairwise Broadphase
local t0 = os.clock()
local naive_pairs = 0
local r2 = R * R
for i = 1, N do
    local xi, yi = entities[i].x, entities[i].y
    for j = i + 1, N do
        local dx = entities[j].x - xi
        local dy = entities[j].y - yi
        if dx * dx + dy * dy <= r2 then
            naive_pairs = naive_pairs + 1
        end
    end
end
local t1 = os.clock()
local naive_time = t1 - t0

-- 2. luapattern O(1) Spatial Hash Grid Broadphase
local t2 = os.clock()
local grid = pat.grid(CELL)
for i = 1, N do
    grid:insert(entities[i].id, entities[i].x, entities[i].y)
end

local grid_pairs = 0
for i = 1, N do
    local xi, yi = entities[i].x, entities[i].y
    local nearby = grid:query(xi, yi, R, true)
    for k = 1, #nearby do
        if nearby[k] > entities[i].id then
            grid_pairs = grid_pairs + 1
        end
    end
end
local t3 = os.clock()
local grid_time = t3 - t2

assert(naive_pairs == grid_pairs, string.format("Mismatch: naive=%d, grid=%d", naive_pairs, grid_pairs))

-- 3. Repetitive Math: Raw vs Memoized
local function complex_eval(a)
    return math.sin(a) * math.cos(a * 2.0) + math.sin(a * 3.0) * math.sqrt(math.abs(math.cos(a))) + math.atan(a)
end

local REPS = 500000
local t4 = os.clock()
local raw_sum = 0
for i = 1, REPS do
    local angle = pat.quantize((i % 360) * 0.0174532925, 0.05)
    raw_sum = raw_sum + complex_eval(angle)
end
local t5 = os.clock()
local raw_math_time = t5 - t4

local memo_fn = pat.memo(complex_eval)
local t6 = os.clock()
local memo_sum = 0
for i = 1, REPS do
    local angle = pat.quantize((i % 360) * 0.0174532925, 0.05)
    memo_sum = memo_sum + memo_fn(angle)
end
local t7 = os.clock()
local memo_math_time = t7 - t6

print(string.format("=== luapattern Benchmark (N=%d entities, R=%.1f) ===", N, R))
print(string.format("Pairs Found: %d", grid_pairs))
print(string.format("Naive O(N^2) Broadphase:       %.4f s", naive_time))
print(string.format("luapattern O(1) Spatial Grid:  %.4f s  (Speedup: %.2fx)", grid_time, naive_time / grid_time))
print(string.format("Raw Repetitive Math:           %.4f s", raw_math_time))
print(string.format("luapattern O(1) Memoized Math: %.4f s  (Speedup: %.2fx)", memo_math_time, raw_math_time / memo_math_time))
