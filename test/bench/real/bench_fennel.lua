local fennel = require("fennel")
local src = [==[
(fn fib [n] (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2)))))
(local t {})
(each [i (ipairs [1 2 3 4 5 6 7 8 9 10])]
  (tset t i (* i i)))
(fn mymap [f xs] (let [out []] (each [_ x (ipairs xs)] (table.insert out (f x))) out))
(local doubled (mymap (fn [x] (* x 2)) [1 2 3 4 5]))
(fn factorial [n] (if (<= n 1) 1 (* n (factorial (- n 1)))))
(local result {:fib (fib 10) :fact (factorial 5) :doubled doubled})
(print result.fib result.fact)
]==]
local iters = 200
local total = 0
for it = 1, iters do
  local lua = fennel.compileString(src)
  total = (total + #lua) % 1000000007
end
local lua = fennel.compileString(src)
local sum = 0
for i = 1, #lua do sum = (sum * 131 + lua:byte(i)) % 1000000007 end
print("fennel", iters, total, sum, #lua)
