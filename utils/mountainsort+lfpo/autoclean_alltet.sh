let NTET=`wc -l < TEMPLATE.par`-4
for TET in $(seq 0 $NTET);
do
cd tet${TET}/
clu_clean_kde_ratio.py tet${TET}. 48 -1 FIT
cd ..;
done
