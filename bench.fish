set iters $argv[1]
echo "stage: gen"
fish ./gen.fish $argv[2] $argv[3]
set filename gen/$argv[2]-$argv[3].vm
set filesize (wc -c $filename | awk '{print $1}')
set nbytes (math $filesize '*' $iters)
echo "stage: run"
set ntime (/usr/bin/time -f%e ./build/nanovm.exe "*$iters" $filename 2>&1)
set mibps (math -s3 $nbytes / $ntime/ 1024 / 1024)
echo "MiB/s: $mibps ($nbytes bytes) ($ntime seconds)"