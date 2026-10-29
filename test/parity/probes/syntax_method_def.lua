local t = {v = 5}
function t:get()
    return self.v
end
print(t:get())
