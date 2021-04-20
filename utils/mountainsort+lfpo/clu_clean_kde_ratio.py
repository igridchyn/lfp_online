#!/usr/bin/env python3

from sys import argv
import numpy as np
from matplotlib import pyplot as plt
import os
#from iutils_p3 import *
#from call_log import *
from scipy import stats
import scipy.stats as st
from sklearn.neighbors import KernelDensity, KDTree
#from sklearn.grid_search import GridSearchCV
from sklearn.model_selection import GridSearchCV
from joblib import dump, load
from sklearn.preprocessing import StandardScaler
import multiprocessing
from scipy.interpolate import UnivariateSpline

def grid_search(data, space):
    grid = GridSearchCV(KernelDensity(kernel='gaussian'), {'bandwidth': space}, cv=5, n_jobs=-1)
    return grid.fit(data).best_params_['bandwidth']
    
def parallel_score_samples(kde, samples, thread_count=int(0.95 * multiprocessing.cpu_count())):
    with multiprocessing.Pool(thread_count) as p:
        return np.concatenate(p.map(kde.score_samples, np.array_split(samples, thread_count)))

def onclick(event):
    print('%s click: button=%d, x=%d, y=%d, xdata=%f, ydata=%f' % ('double' if event.dblclick else 'single', event.button, event.x, event.y, event.xdata, event.ydata))
    if event.button != 2:
        print ('CLICK WITH BUTTON 2 FOR CLEANING')
        return

    per_rem = event.xdata
    # remove first per_rem spikes (dirtiest) from cluster, save and exit
    global clu, res, BASE, clean_ord, current_cleaned
    ind_rem = clean_ord[:int(per_rem / 100 * len(clean_ord))];

    if current_cleaned:
        print('THIS CLUSTER WAS CLEANED ALREADY')
        return

    print('DELETE %.2f%% OF CLUSTER (%d SPIKES)' % (per_rem, len(ind_rem)))

    cind_rem = np.where(clu == c)[0][ind_rem]
    clu[cind_rem] = -1
    current_cleaned = True

    plt.close()

#    clu = np.delete(clu, cind_rem)
#    res = np.delete(res, cind_rem)

#    np.savetxt(BASE + 'clu', clu, fmt='%d')#, skiprows=1)
#    np.savetxt(BASE + 'res', res, fmt='%d')

#=======================================================================================================================================================================================

if len(argv) < 4:
    print('USAGE: (1)<base - clu/res/fet> (2)<refractory> (3)<CLU or -1 for all clusters> (4)<mode: FIT / CLEAN>')
    exit(0)

# build KDE on all spikes
BASE = argv[1]
# TODO: define based on AC
REFR = int(argv[2])
print('REFRACTORY:', REFR)

if argv[4] not in ['FIT', 'CLEAN']:
    print('LAST ARGUMENT MUST BE "FIT" or "CLEAN"')
    exit(1)

DDIR = 'AUTOCLEAN/'
if not os.path.isdir(DDIR):
    os.mkdir(DDIR)

clu = np.loadtxt(BASE + 'clu', dtype=int)#, skiprows=1)
res = np.loadtxt(BASE + 'res', dtype=int)
fetpath = BASE + 'fet.0'
if os.path.isfile(fetpath + '.npy'):
    fet = np.load(fetpath + '.npy')
else:
    fet = np.loadtxt(fetpath, dtype=int, skiprows=1)
    np.save(fetpath + '.npy', fet)
print('LOADED')

# FILTER OUT THOSE NOT IN RES (RES ONLY CONTAINS CLUSTERED SPIKES)
inds = []
ires = 0
for i in range(0, len(fet)): # 0/1 ?
    while ires < len(res) and res[ires] < fet[i,-1]:
        ires += 1

    if ires == len(res):
        break

    if fet[i,-1] == res[ires]:
        inds.append(i)
        ires += 1

inds = np.array(inds)
# TODO: use dimensionality
fet = fet[inds,:(fet.shape[1] - 5)]
nchan = fet.shape[1] // 2

print('DONE FET FILTERING; LEN FET/RES', len(fet), len(res))

#SHOW_SCAT = False
SHOW_SCAT = True
BW_ONLY = argv[4] == 'FIT'

carg = int(argv[3])
clist = [carg] if carg > 0 else range(2, max(clu)+1)
for c in clist:
    print('PROCESS CLUSTER %d' % c)
    # CHECK FET
    fetc = fet[clu == c]
    print('FET IN CLU', len(fetc))

    if len(fetc) < 200:
        print('IGNORE CLUSTER WITH < 200 SPIKES')
        continue

    # MUCH WORSE CLEANING OUTCOME
    #fetc = StandardScaler().fit_transform(fetc)

    # STANDARDIZE? WILL REBALANCE PCA IMPORTANCE!
    # VALIDATE STANDARDIZATION
    #plt.scatter(fetc[:,0], fetc[:,2], s=2)
    #plt.show()

    # find set of refractory
    # TODO: INCORPORATE INTO ALGORITHM INFORMATION THAT P_r(i) + P_r(i-j) = 1, that is only one of the two is refractory
    resc = res[clu == c]
    ref_ind = np.zeros((len(resc)), dtype=int)
    ref_pairs = {}
    for i in range(1, len(resc)):
        j = 1
        while i-j>=0 and resc[i] - resc[i-j] < REFR:
            ref_ind[i-j] = 1
            ref_ind[i] = 1    

            if i-j in ref_pairs:
                ref_pairs[i-j].append(i)
            else:        
                ref_pairs[i-j] = [i]

            if i in ref_pairs:
                ref_pairs[i].append(i-j)
            else:
                ref_pairs[i] = [i-j]

            j+=1

    print('TOTAL REFRACTORY', np.sum(ref_ind), ' OUT OF ', len(ref_ind))

    ref_ind = ref_ind.astype(bool)

    if np.sum(ref_ind)/len(ref_ind)*100 < 0.3 or np.sum(ref_ind) < 10:
        print("BINGO: <0.3%% REFRACTORY SPIKES OR REF SPIKES < 10, IGNORE CLUSTER")
        continue

    # TODO: calculate for every cluster or FIX, may be depending on size?
    # ONLY GAUSSIAN WORKS
    KERNEL = 'gaussian' # [‘gaussian’|’tophat’|’epanechnikov’|’exponential’|’linear’|’cosine’]
    # was .1 - 1.0; orig: 30 steps, 20 CV

    BWPATH = DDIR + 'badnwidth_C%d.txt' % c
    if not os.path.isfile(BWPATH):
        # TODO : DEFINE SEARCH RANGE BASED ON NUMBER OF DATA POINTS

        if not BW_ONLY:
            print('!!! WARNING: NO BANDWIDTH FOUND FOR CLUSTER, CONTINUE')
            continue

        print('GRID SEARCH KDE BANDWIDTH')
        MAX_GRID = 50000
        if len(fetc) > MAX_GRID:
            sub_inds =  np.random.choice(len(fetc), MAX_GRID, replace=False)
            fet_grid = fetc[sub_inds,:]
        else:
            fet_grid = fetc

        bw_start = grid_search(fet_grid, [250,750])
        range1 = 0 if bw_start == 250.0 else 1
        print('SEED BANDWIDTH:', bw_start)
        ranges = [np.linspace(100,500,20), np.linspace(500,1000,20)]        

        bw = grid_search(fet_grid, ranges[range1])
        bw_ref = grid_search(fetc[ref_ind,:], np.logspace(3,3.7,100))

        if bw == 1000.0:
            print('WARNING: BANDWIDTH OUT OF RANGE (>1000), PERFORM SEARCH IN RANGE 1000-1500')
            bw = grid_search(fet_grid, np.linspace(1000, 1500, 20))

        if bw == 500.0:
            print('BANDWIDTH OUT OF RANGE, PERFORM SEARCH IN OPPOSITE RANGE (SEED MISSED)')
            bw = grid_search(fet_grid, ranges[1-range1])
           
        if bw == 100.0:
            print('WARNING: BANDWIDTH OUT OF RANGE (<100), PERFORM SEARCH IN RANGE 10-100')
            bw = grid_search(fet_grid, np.linspace(10,100,20))

        if bw_ref == 1000.0:
            print('BANDWIDTH (REF) OUT OF RANGE, PERFORM SEARCH 10^2.5-10^3')
            bw_ref = grid_search(fetc[ref_ind,:], np.logspace(2.5,3,100))
            
        print('BEST BW: %.2f' % bw)
        print('BEST BW REF: %.2f' % bw_ref)

        np.savetxt(BWPATH, [bw, bw_ref])
    else:
        [bw, bw_ref] = np.loadtxt(BWPATH)

    # OPT: EXCLUDE REFRACTORY FROM MAIN KDE !

    #MAX_KDE = 55000
    MAX_KDE = 50000

    PATH_SCORES = DDIR + 'kde_scores_C%d.txt' % c
    PATH_SCORES_REF = DDIR + 'kde_scores_C%d_ref.txt' % c
    if os.path.isfile(PATH_SCORES):
        print('LOAD SCORES')
        f = np.loadtxt(PATH_SCORES)
        f_ref = np.loadtxt(PATH_SCORES_REF)
    else:
        print('FIT KDE AND CALCULATE LIKELIHOODS')
        # was 0.2
        kde = KernelDensity(kernel=KERNEL, bandwidth=bw).fit(fetc)
        #f = kde.score_samples(fetc)

        if len(fetc) < MAX_KDE:
            fetc_sub = fetc
            subsampled = False
        else:
            #fetc_sub = fetc[::10, :]
            sub_inds =  np.random.choice(len(fetc), MAX_KDE, replace=False)
            fetc_sub = fetc[sub_inds, :]
            subsampled = True

        #f = parallel_score_samples(kde, fetc)
        f_sub = parallel_score_samples(kde, fetc_sub)
        kde_ref = KernelDensity(kernel=KERNEL, bandwidth=bw_ref).fit(fetc[ref_ind,:])
        #f_ref = kde_ref.score_samples(fetc)
        f_ref = parallel_score_samples(kde_ref, fetc)

        if (subsampled):
            # BUILD TREE TO ASSIGN SAME KDE VALUE!
            fet_tree = KDTree(fetc_sub, leaf_size=2)
            # USE TREE TO ASSIGN SCORES
            dist, ind = fet_tree.query(fetc, k=1) 
            f = f_sub[ind].reshape(f_ref.shape)
        else:
            f = f_sub

        # SAVE SCORES
        np.savetxt(PATH_SCORES, f)
        np.savetxt(PATH_SCORES_REF, f_ref)

    if BW_ONLY:
        continue

    # TODO ? REMOVE IF NOT USING DISPLAY ? !!!
    # TODO tune linear combination to maximize cleanup effect ? < few different weights were worse than simple diff
    #f_ref[f_ref < -100] = -100
    #f_ref[f_ref < -75] = -75
    SC_THOLD = -55 if nchan == 3 else -75 # -75 for 3 chan
    f_ref[f_ref < SC_THOLD] = SC_THOLD
#    f[f < SC_THOLD] = SC_THOLD
    f_rat = f_ref - f

    # CALCULATE: CLEANING CURVE = % REFRACTORY AS FUNCTION OF % DELETED
    clean_ord = np.argsort(f_rat)[::-1]
    # NOW DELETE 1-BY-1 AND CALCULATE CLEANINGNESS
    # THE COMPLICATED LOGIC OF REFRACTORY ACCOUNTING IS DUE TO POSSIBGILITY OF REFRACTORY TRIPLETS AND MORE
    dirt = np.sum(ref_ind)
    tot = len(fetc)
    clean = [dirt / tot * 100]
    #for i in range(len(clean_ord)-10) :
    for i in range(len(clean_ord)//2) :
        clean_id = clean_ord[i]
        if clean_id in ref_pairs:
            for pair_id in ref_pairs.pop(clean_id):
                if len(ref_pairs[pair_id]) == 1:
                    ref_pairs.pop(pair_id)
                    dirt -= 1
                else:
                    ref_pairs[pair_id].remove(clean_id)
            dirt -= 1
        tot -= 1
        clean.append(dirt / tot * 100)

    #print('CLEANEST:', np.min(clean), ' AT ', np.argmin(clean)/len(fetc))

    if SHOW_SCAT:
        MAX_DISP = 10000
        if len(fetc) > MAX_DISP:
            sub_inds =  np.random.choice(len(fetc), MAX_DISP, replace=False)
            print('PLOT RANDOM SUBSET')
            fetc_draw = fetc[sub_inds,:]
            f_rat_draw = f_rat[sub_inds]
        else:
            fetc_draw = fetc
            f_rat_draw = f_rat

        fig, axar = plt.subplots(3, 2, figsize=(20,10))
        perms = [((0,2), (0,4), (0,6), (2,4), (2,6), (4,6)), ((0,2), (0,4), (2,4), (1, 3), (1, 5), (3, 5)), ((0,1),(0,2),(0,3),(1,2),(1,2),(0,2))][4-nchan]
        for ia in range(6):
            #axar[ia % 3, ia // 3].scatter(fetc[:,perms[ia][0]], fetc[:,perms[ia][1]], s=2, c=clean_ord, cmap='jet')#, color='black')
            axar[ia % 3, ia // 3].scatter(fetc_draw[:,perms[ia][0]], fetc_draw[:,perms[ia][1]], s=2, c=f_rat_draw, cmap='jet')#, color='black')
            axar[ia % 3, ia // 3].tick_params(labelbottom=False, labelleft=False)
        plt.tight_layout()
        plt.show()    

    # DOWNSAMPLE IF TOO LARGE - 2K POINTS ENOUGH FOR ACCURATE SELECTION
    if len(clean) > 2000:
        smpl = len(clean) // 2000
        clean = clean[::smpl]

    # CALCULATE 2ND DERIVATIVE
    cc_x=np.arange(0, 80, 80/len(clean))
    cc_y=clean
    cc_x=cc_x[:len(cc_y)]
    #cc_spl = UnivariateSpline(cc_x,cc_y,s=0,k=4)
    #cc_spl_2d = cc_spl.derivative(n=2)
    #clean_2diff = np.diff(clean, n=2)

    # FIND BIGGEST DROP IN 0.05% INCREMENTS
    cl_step = max(int(0.005 * len(clean)), 1)
    clean = np.array(clean)
    print('DEBUG:', clean.shape, cl_step)
    cl_diffs = -(clean[cl_step:] - clean[:-cl_step])

    # CUTTING POINT: MIN CLEANINGNESS + cl_diffs ABOVE THRESHOLD / SHARP DROP

    # PLOT: scatter with color = DELETION RANK + CLEANING CURVE
    current_cleaned = False
    fig=plt.figure(figsize=(15,15))
    plt.plot(cc_x, cc_y)
    cid = fig.canvas.mpl_connect('button_press_event', onclick)

    #plt.plot(cc_x[1:-1], clean_2diff)
    #plt.plot(cc_x, cc_spl_2d(cc_x))
    ax1 = plt.gca()
    LFS = 20
    plt.xlabel('Dirtiest spikes removed, %', fontsize=LFS)
    plt.ylabel('Refractory spikes content, %', fontsize=LFS)
    ax2 = plt.gca().twinx()
    ax2.plot(cc_x[cl_step:], cl_diffs, color='red')
    plt.title('CLEANING CLUSTER %d (REF %d OUT OF %d), %.2f HZ' % (c, np.sum(ref_ind), len(ref_ind), len(ref_ind)*24000/res[-1]), fontsize = LFS+5)
    plt.show()
    #exit(0)

    # DEBUIG - FIND THRESHOLD FOR F
    #plt.hist(f[f>fper])
    #plt.hist(f_rat)
    #plt.show()

    #cfset = plt.contourf(xx, yy, f, cmap='Blues')
    #cfset_ref = plt.contourf(xx, yy, f_ref, cmap='Blues', levels = 30)
    #cfset_ref = plt.contourf(xx, yy, f_rat, cmap='Blues', levels = 30)
    #cfset_ref = plt.contourf(fetc[:,0], fetc[:,1], f_rat, cmap='Blues', levels = 30)

cind_rem = np.where(clu == -1)[0]
clu = np.delete(clu, cind_rem)
res = np.delete(res, cind_rem)
np.savetxt(BASE + 'clean.clu', clu, fmt='%d')
np.savetxt(BASE + 'clean.res', res, fmt='%d')
