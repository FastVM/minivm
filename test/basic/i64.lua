local tab = {}

tab.remain = 92
tab.cur = 0
tab.next = 1

while tab.remain ~= 0 do
    tab.old = tab.cur
    tab.cur = tab.next
    tab.next = tab.old + tab.cur
    tab.remain = tab.remain - 1
end

if tab.cur % 10 == 9 then
    print("use i64")
else
    print("use f64")
end
