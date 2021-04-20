DPATH=$1
rsync -avP --exclude="*spk.0" --exclude="*mda*" $DPATH/tet* .
rsync -avP --exclude="*spk.0" --exclude="*mda*" $DPATH/BASELIST .
rsync -avP --exclude="*spk.0" --exclude="*mda*" $DPATH/TEMPLATE.par .
rsync -avP $DPATH/tet0/tet0.spk.0.ORIG tet0/tet0.spk.0
