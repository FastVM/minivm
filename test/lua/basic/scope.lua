assert(r == nil, "global does not exist before set")
r = 1
do
    local r = 2
    assert(r == 2, "global took local priority")
    do
        r = 3
        assert(r == 3, "can't reassign right")
        do
            local r = 4
            assert(r == 4, "can't shadow correctly")
            do
                assert(r == 4, "inner took priority too early")
                local r = 5
                assert(r == 5, "inner didn't take priority")
            end
            assert(r == 4, "local escaped")
        end
        assert(r == 3, "mutation")
    end
end
assert(r == 1, "r should be 1")

print('test/basic/scopes: pass')
