#!/bin/bash

# was 20000
ml-run-process ephys.bandpass_filter --inputs timeseries:tet$1/tet${1}raw.mda --parameters samplerate:24000 freq_min:300 freq_max:6000 --outputs timeseries_out:tet$1/raw_filt.mda

ml-run-process ephys.whiten --inputs timeseries:tet$1/raw_filt.mda --outputs timeseries_out:tet$1/pre.mda

# TRY TO FEED OUR FEATURES HERE !
ml-run-process ms4alg.sort --inputs timeseries:tet$1/pre.mda geom:tet$1/geom.csv --outputs firings_out:tet$1/firings.mda --parameters adjacency_radius:-1 detect_sign:-1 detect_threshold:$2

