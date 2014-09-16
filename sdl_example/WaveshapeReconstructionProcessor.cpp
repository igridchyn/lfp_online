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

void WaveShapeReconstructionProcessor::find_one_peak(Spike* spike, int *ptmout,int peakp,int peakgit,int *ptmval){
    int pmax,ptm,i,j;
    // !!! why channel 0 ???
    pmax=spike->waveshape[0][peakp];
    ptm=peakp;
    
    for(i=0;i<spike->num_channels_;i++) {
        for(j=peakp-peakgit;j<peakp+peakgit;j++)  {
            if (j<0) continue;
            if (spike->waveshape[i][j] < pmax){
                ptm=j;
                pmax=spike->waveshape[i][j];
            }
        }
    }
    *ptmout=ptm;
    *ptmval=pmax;
}

WaveShapeReconstructionProcessor::WaveShapeReconstructionProcessor(LFPBuffer *buf)
:WaveShapeReconstructionProcessor(buf,
		buf->config_->getInt("waveshape.rec.mul")
		){}

WaveShapeReconstructionProcessor::WaveShapeReconstructionProcessor(LFPBuffer* buffer, int mul)
:LFPProcessor(buffer)
, mul(mul)
, cleanup_ws_(buffer->config_->getBool("waveshape.cleanup", false)){
    // create lookup tables
    construct_lookup_table();
}

void WaveShapeReconstructionProcessor::process(){
    // make sure alignment has been performed
    while (buffer->spike_buf_no_rec < buffer->spike_buf_nows_pos){
        Spike *spike = buffer->spike_buffer_[buffer->spike_buf_no_rec];
        if (!(spike->aligned_ || spike->discarded_)){
            break;
        }
        
        if (spike->discarded_){
            buffer->spike_buf_no_rec++;
            continue;
        }
        
        load_restore_one_spike(spike);
        
        // DEBUG: reconstructed waveshape, channel 0
//        printf("Rec ws: ");
//        for (int i=0; i < 128; ++i){
//            printf("%d ", buffer->spike_buffer_[buffer->spike_buf_no_rec]->waveshape[0][i]);
//        }
//        printf("\n");
        
        // find peak
        int peak_time, peak_value;
        find_one_peak(spike, &peak_time, 64, 4, &peak_value);
        
        // form final waveshape

        spike->waveshape_final = new int*[spike->num_channels_];
        for(int i=0;i<spike->num_channels_;i++) {
            spike->waveshape_final[i] = new int[16];
            for(int j=0;j<16;j++) {
                // TODO: calclulate density, shift
                spike->waveshape_final[i][j] = spike->waveshape[i][peak_time-32+j*4];
                // eigpoi[j]=avb[i][ptm-eis_kk+j*eisk];
            }
        }
        
        // TODO separate cleanup of final and intermediate extended
        if (cleanup_ws_){
        	for(int i=0;i<spike->num_channels_;i++) {
        		delete spike->waveshape[i];
        		spike->waveshape[i] = NULL;
        	}
        	delete spike->waveshape;
        	spike->waveshape = NULL;
        }

        // DEBUG
//        for (int i=0; i < spike->num_channels_; ++i){
//            printf("Final WS, channel #%d :", i);
//            for (int j=0; j < 16; ++j){
//                printf("%d ", spike->waveshape_final[i][j]);
//            }
//            printf("\n");
//        }
        
        buffer->spike_buf_no_rec++;
    }
}
