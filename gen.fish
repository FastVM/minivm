set a $argv[1]
set b $argv[2]
begin
    echo "(block"
    echo "  (define n 1)"
    set -a words (cat /usr/share/dict/american-english)
    for i in (seq $a)
        for j in (seq $b)
            set word $words[$j]
            echo "  (define fun" \
                "(function (arg1-$i arg2-$j)" \
                "(block" \
                "(define noop-$word (< arg1-$i arg2-$j))" \
                "(define ret-$i-$j (+ arg1-$i arg2-$j))" \
                "(* (- ret-$i-$j)))))" \
                "(define n (- (/ (fun $j) n) (fun $i)))"
        end
    end
    echo "  n)"
end >gen/"$a-$b".vm