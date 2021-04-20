let NTET=`wc -l < TEMPLATE.par`-4
for TET in $(seq 0 ${NTET}); do
echo "RUN WAVESHAPE INTERPOLATION FOR TETRODE " $TET
let NCH=(`head -1 tet${TET}/*fet*`-5)/2
ws_interpolate $NCH tet${TET}/tet${TET}.spk.${TET}; mv test.out tet${TET}/tet${TET}.spk.0;
done
