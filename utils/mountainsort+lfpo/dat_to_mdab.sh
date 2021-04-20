#!/bin/bash

[ ! -d "tet$2" ] && mkdir tet$2

# create tetrode par
NUM=$2
sed "$((NUM+4))q;d" $1.par | awk '{print 64, $1, 50}' >  tet$2/tet${2}.par.$2
sed "$((NUM+4))q;d" $1.par | awk '{$1=""; print $0}' >>  tet$2/tet${2}.par.$2
cat TEMPLATE.par.tet >> tet$2/tet${2}.par.$2

NCHAN=`sed "$((NUM+4))q;d" $ANIMAL-$DAY.par | awk '{print $1}'`
echo $NCHAN > nchan.tmp

cp $1.par tet$2/tet${2}.par

#cp $1.par.$2 tet$2/tet${2}.par.$2

head -$NCHAN geom.csv > tet$2/geom.csv

[ -f "tet$2/tet$2raw.mda" ] && echo "WARNING: raw.mda exists, skip conversion" || Dat_to_Mda.py $1 $2 $3 $4
#python3 Dat_to_Mda.py $1 $2 $3
