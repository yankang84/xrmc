for th in $(seq -90 10 90); do
    printf "%d\t" $th
    for ir in $(seq 1 10); do
        x=$(extract/extract d_${ir}_${th}/output_${ir}_${th}.dat)
        printf "%g\t" $x
    done
    echo
done > plot.dat
