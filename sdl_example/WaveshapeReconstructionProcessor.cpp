//
//  WaveshapeReconstructionProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 15/04/14.
//  Copyright (c) 2014 Igor Gridchyn. All rights reserved.
//

#include "LFPProcessor.h"

void WaveShapeReconstructionProcessor::construct_lookup_table(){
    int nosm = 32;
    
    int i,j,k,l;
    double t,di,ns;
    sin_table= (double *) calloc(nosm*mul,sizeof(double));
    t_table= (double *) calloc(nosm*mul,sizeof(double));
    it_table= (int *) calloc(nosm*mul,sizeof(int));
    sztable = (double **) calloc(nosm*mul,sizeof(double *));
    for (i=0;i<nosm*mul;i++) {
        if (!(*(sztable + i) = (double *) calloc(nosm,sizeof(double)))) {
            fprintf(stderr,"Memory allocation error!! \n");
            exit(-1);
        };
    }
    
    ns = (double)nosm;
    
    for(i=0,j=0;i<nosm;i++) {
        for(k=0;k<mul;k++,j++) {
            t=i+(double)k/mul;
            *(sin_table+j) = sin(t*M_PI);
            *(t_table+j) = t;
            *(it_table+j) = (int)t;
            
            for (l=0;l<nosm;) {
                di = t-l;
                /* even side */
                *(*(sztable+j)+l) =  1.0/(di+ns) + 1.0/di + 1.0/(di-ns);
                /* odd side */
                di = ++l-t;
                *(*(sztable+j)+l++) =  1.0/(di+ns) + 1.0/di + 1.0/(di-ns);
            }
        }
    }
    
    // DEBUG
//    printf("sin_table:\n");
//    for(i=0,j=0;i<nosm;i++) {
//        for(k=0;k<mul;k++,j++) {
//            printf("%lf ", sin_table[j]);
//        }
//    }
//    printf("\nsz:\n");
//    for(i=0,j=0;i<nosm;i++) {
//        for(k=0;k<mul;k++,j++) {
//            for (l=0;l<nosm;l++) {
//                printf("%lf ", sztable[j][l]);
//            }
//        }
//    }
    
}

int WaveShapeReconstructionProcessor::optimized_value(int num_sampl,int *sampl,int h){
    /* assumes even num_sampl */
    int i,it;
    double sz1,ns,sina,t;
    ns = num_sampl; /* make it double! */
    
    t = t_table[h];
    // only to see if t==it (integer time point)
    it = it_table[h];
    
    // if time point is at a sample, also, Sin_table is 0 here
    if (t-it==0.0) {
        return sampl[it];
    }
    
    sina=sin_table[h];
    
    for(i=0,sz1=0.0;i<num_sampl;i++) {
        sz1+=sampl[i] * sztable[h][i]; // sz - weight of i-th sample in computing value at the h-th time point
    }
    return (int)(sina*sz1/M_PI);
}

void WaveShapeReconstructionProcessor::load_restore_one_spike(Spike *spike){
    
    // reconstruct using interpolation of mul-1 values in-between sampled ones
    for(int i=0,h=0;i<32;i++) {
        for(int k=0;k<mul;k++,h++) {
            for(int j=0;j<spike->num_channels_;j++) {
                rec_tmp_[j][i*mul+k]=optimized_value(32, spike->waveshape[j], h);
            }
        }
    }
    
    // copy from temporary array to spike object array
    for(int chan = 0; chan < spike->num_channels_; ++chan){
        memcpy(spike->waveshape[chan], rec_tmp_[chan], 128*sizeof(int));
    }
}

WaveShapeReconstructionProcessor::WaveShapeReconstructionProcessor(LFPBuffer* buffer, int mul)
:LFPProcessor(buffer)
, mul(mul){
    // create lookup tables
    construct_lookup_table();
}

void WaveShapeReconstructionProcessor::process(){
    // make sure alignment has been performed
    while (buffer->spike_buf_no_rec < buffer->spike_buf_nows_pos){
        load_restore_one_spike(buffer->spike_buffer_[buffer->spike_buf_no_rec]);
        
        // DEBUG: reconstructed waveshape, channel 0
        printf("Rec ws: ");
        for (int i=0; i < 128; ++i){
            printf("%d ", buffer->spike_buffer_[buffer->spike_buf_no_rec]->waveshape[0][i]);
        }
        printf("\n");
        
        buffer->spike_buf_no_rec++;
    }
}