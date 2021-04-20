dat_to_mdab.sh $ANIMAL-$DAY $TET $NDAT $NCHAN		# todo - simultaneously for all tetrodes
[ -f tet${TET}/firings.mda ] && echo "WARNING: tet${TET}/firings.mda exists, omit mountainsort pipeline" || mount_sortb.sh $TET $DTHOLD                 # (5 default, 3,4 try - see cell and ); 4 - less good cells but better rates;
mountain_to_sgclustb.py $TET
sgclust_to_lfpo.py . $TET 	# replace . with directory containing tet#/ if different from working directory
cp $ANIMAL-$DAY.whl tet$TET/tet${TET}.whl
cp  session_shifts.txt tet$TET/
if [ $TET == 0 ] && [ ! -f tet0/tet0.spk.0.ORIG ]; then cp tet0/tet0.spk.0 tet0/tet0.spk.0.ORIG ; fi
ws_interpolate `cat nchan.tmp` tet${TET}/tet${TET}.spk.${TET}; mv test.out tet${TET}/tet${TET}.spk.0
echo 0 > tet${TET}/tet${TET}.cluster_shifts
