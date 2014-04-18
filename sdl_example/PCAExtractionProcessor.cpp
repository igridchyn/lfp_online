//
//  PCAExtractionProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 15/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))

#include "LFPProcessor.h"

void PCAExtractionProcessor::tred(float **a,int n,float d[],float e[]) {
    int l,k,j,i;
    float scale,hh,h,g,f;
    
    for (i=n;i>=2;i--) {
        l=i-1;
        h=scale=0.0;
        if (l>1) {
            for (k=1;k<=l;k++)
                scale += fabs(a[i][k]);
            if (scale == 0.0)
                e[i]=a[i][l];
            else {
                for (k=1;k<=l;k++) {
                    a[i][k] /=scale;
                    h+=a[i][k]*a[i][k];
                }
                f=a[i][l];
                g = f>0 ? -sqrt(h) : sqrt(h);
                e[i]=scale*g;
                h -= f*g;
                a[i][l]=f-g;
                f=0.0;
                for(j=1;j<=l;j++) {
                    a[j][i]=a[i][j]/h;
                    g=0.0;
                    for (k=1;k<=j;k++)
                        g+= a[j][k]*a[i][k];
                    for (k=j+1;k<=l;k++)
                        g+=a[k][j]*a[i][k];
                    e[j]=g/h;
                    f +=e[j]*a[i][j];
                }
                hh=f/(h+h);
                for (j=1;j<=l;j++) {
                    f=a[i][j];
                    e[j]=g=e[j]-hh*f;
                    for(k=1;k<=j;k++)
                        a[j][k] -= (f*e[k]+g*a[i][k]);
                }
            }
        } else
            e[i]=a[i][l];
        d[i]=h;
    }
    
    d[1]=0.0;
    e[1]=0.0;
    for(i=1;i<=n;i++) {
        l=i-1;
        if (d[i]) {
            for (j=1;j<=l;j++) {
                g=0.0;
                for (k=1;k<=l;k++)
                    g+= a[i][k]*a[k][j];
                for(k=1;k<=l;k++)
                    a[k][j] -= g*a[k][i];
            }
        }
        d[i]=a[i][i];
        a[i][i]=1.0;
        for(j=1;j<=l;j++) a[j][i]=a[i][j]=0.0;
    }
}


void PCAExtractionProcessor::tqli(float d[],float e[],int n,float **z) {
    int m,l,iter,i,k;
    float s,r,p,g,f,dd,c,b;
    void nrerror();
    
    for (i=2;i<=n;i++) e[i-1]=e[i];
    e[n]=0.0;
    for(l=1;l<=n;l++) {
        iter=0;
        do {
            for(m=l;m<=n-1;m++) {
                dd=fabs(d[m])+fabs(d[m+1]);
                if (fabs(e[m])+dd == dd) break;
            }
            if (m != l) {
                if (iter++ == 1000){ printf("too many iteration in TQLI");
                    exit(0); }
                g=(d[l+1]-d[l])/(2.0*e[l]);
                r=sqrt((g*g)+1.0);
                g=d[m]-d[l]+e[l]/(g+SIGN(r,g));
                s=c=1.0;
                p=0.0;
                for (i=m-1;i>=l;i--) {
                    f=s*e[i];
                    b=c*e[i];
                    if (fabs(f) >= fabs(g)) {
                        c=g/f;
                        r=sqrt((c*c)+1.0);
                        e[i+1]=f*r;
                        c *= (s=1.0/r);
                    }
                    else {
                        s=f/g;
                        r=sqrt((s*s)+1.0);
                        e[i+1]=g*r;
                        s *= (c=1.0/r);
                    }
                    g=d[i+1]-p;
                    r=(d[i]-g)*s+2.0*c*b;
                    p=s*r;
                    d[i+1]=g+p;
                    g=c*r-b;
                    for(k=1;k<=n;k++) {
                        f=z[k][i+1];
                        z[k][i+1]=s*z[k][i]+c*f;
                        z[k][i]=c*z[k][i]-s*f;
                    }
                }  
                d[l]=d[l]-p;
                e[l]=g;
                e[m]=0.0;
            }
        } while( m!=l);
    }
}

void PCAExtractionProcessor::eigenc(float **m,float ev[], int ftno) {
    float **z;
    float *d,*e;
    int i,j;
    
    z=(float**)malloc((ftno+1)*sizeof(float*));
    d=(float*)malloc((ftno+1)*sizeof(float));
    e=(float*)malloc((ftno+1)*sizeof(float));
    
    for(i=1;i<=ftno;i++)    {
        z[i]=(float*)malloc((ftno+1)*sizeof(float));
    }
    
    for(i=0;i<ftno;i++)
        for(j=0;j<ftno;j++)
            z[i+1][j+1]=m[i][j];
    
    tred(z,ftno,d,e);
    tqli(d,e,ftno,z);
    for(i=0;i<ftno;i++) {
        ev[i]=d[i+1];
        for(j=0;j<ftno;j++)
            m[i][j]=z[i+1][j+1];
    }
    
    
    for(i=1;i<=ftno;i++) {
        free(z[i]);
    }
    free(z);
    free(d);
    free(e);
}

void PCAExtractionProcessor::final(float **cor,float mea[],int ftno, int num_obj,float **prm, int prno)  {
    int i,j,sei;
    float seg;
    float *ev,sz1,sz2;
    int *ind;
    
    ev=(float*)malloc(ftno*sizeof(float));
    ind=(int*)malloc(ftno*sizeof(int));
    
    printf("\nMeans, num_obj: %d: ", num_obj);
    for (i=0;i<ftno;i++){
        mea[i]=mea[i]/(float)num_obj;
        printf("%f ", mea[i]);
    }
    printf("\n");

    printf("Correlation matrix:\n");
    for (i=0;i<ftno;i++){
        for ( j=i;j<ftno;j++) {
            cor[j][i]=cor[i][j]=cor[i][j]/(float)num_obj-mea[i]*mea[j];
            printf("%f ", cor[j][i]);
        }
        printf("\n");
    }

    printf("Solving Eigen equation...");
    eigenc(cor,ev,ftno);
    printf("finished!\n");
    
    // bubble sort eigenvalues
    sz1=0.;
    for(i=0;i<ftno;i++) {
        ind[i]=i;
        sz1+=ev[i];
    }
    
    for(i=0;i<ftno-1;i++){
        for(j=0;j<ftno-1-i;j++)
            if (ev[j]<ev[j+1]) {
                seg=ev[j];
                ev[j]=ev[j+1];
                ev[j+1]=seg;
                sei=ind[j];
                ind[j]=ind[j+1];
                ind[j+1]=sei;
            }
    }
    
    printf("\nEigenvalues: ");
    for(i=0;i<ftno;i++){
        printf("%f ", ev[i]);
    }
    printf("\n");
    
    for(j=0;j<prno;j++){
        for(i=0;i<ftno;i++)         {
            prm[j][i]=cor[i][ind[j]];
        }
    }
    
    sz2=0.;
    printf("Variances per feature:  ");
    for(j=0;j<prno;j++){
        printf("%5.4f ",ev[j]/sz1);
        sz2+=ev[j];
    }
    
    printf("\nOverall projected variances : %f5.4\n",sz2/sz1);
    
    free(ev);
    free(ind);
    
}

PCAExtractionProcessor::PCAExtractionProcessor(LFPBuffer *buffer, const unsigned int& num_pc, const unsigned int& waveshape_samples)
: LFPProcessor(buffer)
, num_pc_(num_pc)
, waveshape_samples_(waveshape_samples)
, num_spikes(0)
{
    printf("Create PCA extrator...");
    
    const int nchan = 64;
    
    cor_ = new int**[nchan];
    mean_ = new int*[nchan];
    
    corf_ = new float*[waveshape_samples_];
    for (int w=0; w<waveshape_samples_; ++w) {
        corf_[w] = new float[waveshape_samples_];
        memset(corf_[w], 0, sizeof(float)*waveshape_samples_);
    }
    
    meanf_ = new float[waveshape_samples_];
    memset(meanf_, 0, sizeof(float)*waveshape_samples_);
    
    for (int c=0; c<nchan; ++c) {
        cor_[c] = new int*[waveshape_samples_];
        for (int w=0; w<waveshape_samples_; ++w) {
            cor_[c][w] = new int[waveshape_samples_];
            memset(cor_[c][w], 0, sizeof(int)*waveshape_samples_);
        }
        mean_[c] = new int[waveshape_samples_];
        memset(mean_[c], 0, sizeof(int)*waveshape_samples_);
    }
    
    pc_transform_ = new float**[nchan];
    for (int c=0;c<nchan; ++c) {
        pc_transform_[c] = new float*[waveshape_samples_];
        for (int w=0; w<waveshape_samples_; ++w) {
            pc_transform_[c][w] = new float[num_pc_];
            memset(pc_transform_[c][w], 0, num_pc_ * sizeof(float));
        }
    }
    
    printf("done\n");
}

void PCAExtractionProcessor::compute_pcs(Spike *spike){
    int numchan = buffer->tetr_info_->number_of_channels(spike);
    if (spike->pc == NULL){
        spike->pc = new float*[numchan];
        for (int ci=0; ci < numchan; ++ci) {
            spike->pc[ci] = new float[num_pc_];
        }
    }
    
    for (int c=0; c < numchan; ++c) {
        int chan = buffer->tetr_info_->tetrode_channels[spike->tetrode_][c];
        for (int pc=0; pc < num_pc_; ++pc) {
            spike->pc[c][pc] = 0;
            for (int w=0; w < waveshape_samples_; ++w) {
                spike->pc[c][pc] += spike->waveshape_final[c][w] * pc_transform_[chan][w][pc];
            }
        }
    }
}

void PCAExtractionProcessor::process(){
    // TODO: review logic
    // CURRENT LOGIC: PCA is computed once after required number of spikes has been collected,
    //  computed for all spikes and all new-coming
    
    while (buffer->spike_buf_pos_unproc_ < buffer->spike_buf_no_rec){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_pos_unproc_];
        
        if (spike->discarded_){
            buffer->spike_buf_pos_unproc_++;
            continue;
        }
        
        if (!pca_done_){
            for (int chani = 0; chani < buffer->tetr_info_->number_of_channels(spike); ++chani) {
                int chan = buffer->tetr_info_->tetrode_channels[spike->tetrode_][chani];
                
                for (int w=0; w < waveshape_samples_; ++w) {
                    mean_[chan][w] += spike->waveshape_final[chani][w];
                    
                    if (chani==0 && abs(spike->waveshape_final[chani][w]) > 5000){
                        // printf("Large amplitude at %d: %d!\n",spike->pkg_id_, spike->waveshape_final[chani][w]);
                    }
                        
                    for (int w2=w; w2 < waveshape_samples_; ++w2) {
                        cor_[chan][w][w2] += spike->waveshape_final[chani][w] * spike->waveshape_final[chani][w2];
                    }
                }
            }
        }
        else{
            // compute PCs
            if (!spike->discarded_){
                compute_pcs(spike);
            }
        }
        
        num_spikes++;
        // TODO: account for buffer rewind
        buffer->spike_buf_pos_unproc_++;
    }
    
    // TODO: when to redo PCA?
    // TODO: tetrode-wise counting !!! and check
    if (num_spikes >= 300 && !pca_done_){
        for (int channel = 0; channel < 64; ++channel){
            if (!buffer->is_valid_channel(channel)){
                continue;
            }
            
            // copy cor and mean to float arrays
            for (int w = 0; w < waveshape_samples_; ++w){
                meanf_[w] = mean_[channel][w];
                for (int w2 = w; w2 < waveshape_samples_; ++w2){
                    corf_[w][w2] = (float)cor_[channel][w][w2];
                    corf_[w2][w] = cor_[channel][w][w2];
                }
            }
            
            // prm - projection matrix, prm[j][i] = contribution of j-th wave feature to i-th PC
            final(corf_, meanf_, waveshape_samples_, num_spikes, pc_transform_[channel], num_pc_);
            
            // DEBUG - print PCA transform matrix
            if (channel == 8){
                for (int pc=0; pc < 3; ++pc) {
                    printf("PC #%d: ", pc);
                    for (int w=0; w < waveshape_samples_; ++w) {
                        printf("%.2f ", pc_transform_[8][pc][w]);
                    }
                    printf("\n");
                }
            }
            
        }
        
        pca_done_ = true;
        
        // get PCs for all past spikes
        for (int s=0; s < buffer->spike_buf_pos_unproc_; ++s) {
            Spike *spike = buffer->spike_buffer_[s];
            if (spike == NULL || spike->discarded_){
                continue;
            }
            
            compute_pcs(spike);
            
            // DEBUG            
            for (int ci=0; ci < 4; ++ci) {
                printf("PCs %d chan #%d ", spike->pkg_id_, ci);
                for (int pc=0; pc < num_pc_; ++pc) {
                    printf("%f ", spike->pc[ci][pc]);
                }
                printf("\n");
            }
        }
    }
}