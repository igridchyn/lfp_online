if [ "$#" -ne 4 ]; then
    echo "ERROR: number of parameters"
    echo "	USAGE: (1)<ANIMAL> (2)<DAY> (3)<DETECTION THRESHOLD> (4)<64/128 CHANNELS>"
	exit 3
fi

set -e

[ ! -f BASELIST ] && { echo "ERROR: FILE BASELIST NOT FOUND"; exit 1; }
[ ! -f TEMPLATE.par ] && { echo "ERROR: FILE TEMPLATE.par NOT FOUND"; exit 2; }

export ANIMAL=$1 DAY=$2 DTHOLD=$3 NCHAN=$4
export NDAT=`cat BASELIST | wc -l`;
cp TEMPLATE.par $ANIMAL-$DAY.par
[ -f "$ANIMAL-$DAY.whl" ] && echo "WATNING: whl exists, don't run axtrk conversion" || axtrk_to_whl.py BASELIST $NCHAN $ANIMAL-$DAY.whl
echo $NDAT >> $ANIMAL-$DAY.par
cat BASELIST >> $ANIMAL-$DAY.par
let DIV=NCHAN*2
export DIV
ls -lU `awk '{print $1".dat"}' BASELIST` | awk '{sum+=$5/'$DIV'; $5=sum ; print $5}' > session_shifts.txt
let NTET=`wc -l < TEMPLATE.par`-4
for TET in $(seq 0 $NTET); do export TET; echo "PROCESS TET" $TET; ms_lfpo_proc_tet.sh; done
