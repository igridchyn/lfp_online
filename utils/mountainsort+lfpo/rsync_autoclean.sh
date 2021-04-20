let NTET=`wc -l < TEMPLATE.par`-4
DDIR=$1
for TET in $(seq 0 ${NTET});
do
echo $TET
cd tet${TET}/
rsync -avP $DDIR/tet${TET}/AUTOCLEAN .
cd ..;
done
