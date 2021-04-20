# merge clu/res files
BASE=$1
[ -f $BASE.clu ] && echo "ERROR: "$BASE".clu exists, abort merge" && exit || ms_merge_clures.py ./ $BASE.

# create symbolic linkds to fets
cd tet0/
let NTET=`wc -l < ../TEMPLATE.par`-4
for TET in $(seq 1 ${NTET}); do ln -s ../tet${TET}/tet${TET}.fet.0 tet0.fet.${TET}; done
cd ..

# backup tet0 clu/res
[ -f tet0/tet0_SINGLE.clu ] && echo "Single tet0_SINGLE.clu exists, do not backup" || cp tet0/tet0.clu tet0/tet0_SINGLE.clu
[ -f tet0/tet0_SINGLE.res ] && echo "Single tet0_SINGLE.res exists, do not backup" || cp tet0/tet0.res tet0/tet0_SINGLE.res

# copy merged to tet0
cp $BASE.clu tet0/tet0.clu
cp $BASE.res tet0/tet0.res
cp $BASE.cluster_shifts tet0/tet0.cluster_shifts

# whl
cp tet0/tet0.whl $BASE.whl

convert_par_to_tetr_conf.py TEMPLATE.par tetr_$BASE.conf
