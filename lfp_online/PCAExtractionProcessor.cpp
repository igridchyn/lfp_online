//
//  PCAExtractionProcessor.cpp
//  sdl_example
//
//  Created by Igor Gridchyn on 15/04/14.
//  Copyright (c) 2014 Jozsef Csicsvari, Igor Gridchyn. All rights reserved.
//

#define SIGN(a,b) ((b)<0 ? -fabs(a) : fabs(a))

#include <assert.h>

#include "LFPProcessor.h"
#include "PCAExtractionProcessor.h"

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


int PCAExtractionProcessor::tqli(float d[],float e[],int n,float **z) {
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
                    return 1; }
                g=(d[l+1]-d[l])/(2.0f*e[l]);
                r=sqrt((g*g)+1.0f);
                g=d[m]-d[l]+e[l]/(g+SIGN(r,g));
                s=c=1.0f;
                p=0.0;
                for (i=m-1;i>=l;i--) {
                    f=s*e[i];
                    b=c*e[i];
                    if (fabs(f) >= fabs(g)) {
                        c=g/f;
                        r=sqrt((c*c)+1.0f);
                        e[i+1]=f*r;
                        c *= (s=1.0f/r);
                    }
                    else {
                        s=f/g;
                        r=sqrt((s*s)+1.0f);
                        e[i+1]=g*r;
                        s *= (c=1.0f/r);
                    }
                    g=d[i+1]-p;
                    r=(d[i]-g)*s+2.0f*c*b;
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
                e[m]=0.0f;
            }
        } while( m!=l);
    }

    return 0;
}

int PCAExtractionProcessor::eigenc(float **m,float ev[], int ftno) {
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
    int tqli_ret = tqli(d,e,ftno,z);

    if(tqli_ret != 0)
    	return tqli_ret;

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

    return 0;
}

int PCAExtractionProcessor::final(float **cor,float mea[],int ftno, int num_obj,float **prm, int prno)  {
    int i,j,sei;
    float seg;
    float *ev,sz1,sz2;
    int *ind;
    
    ev=(float*)malloc(ftno*sizeof(float));
    ind=(int*)malloc(ftno*sizeof(int));
    
    //printf("\nMeans, num_obj: %d: ", num_obj);
    for (i=0;i<ftno;i++){
        mea[i]=mea[i]/(float)num_obj;
        //printf("%f ", mea[i]);
    }
    //printf("\n");

    //printf("Correlation matrix:\n");
    for (i=0;i<ftno;i++){
        for ( j=i;j<ftno;j++) {
            cor[j][i]=cor[i][j]=cor[i][j]/(float)num_obj-mea[i]*mea[j];
            //printf("%f ", cor[j][i]);
        }
        //printf("\n");
    }
    
    // !!! turn to correlations instead of covariances
//    for (i=0;i<ftno;i++){
//        for ( j=i+1;j<ftno;j++) {
//            cor[j][i]=cor[i][j]=cor[i][j]/(float)sqrt(cor[i][i] * cor[j][j]);
//        }
//    }
//    for (i=0;i<ftno;i++){
//        cor[i][i] = 1.0f;
//    }
    
    
    //printf("Solving Eigen equation...");
    int eigenc_ret = eigenc(cor,ev,ftno);
    if (eigenc_ret != 0){
    	return eigenc_ret;
    }
    //printf("finished!\n");
    
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
    
    // DEBUG
//    printf("\nEigenvalues: ");
//    for(i=0;i<ftno;i++){
//        printf("%f ", ev[i]);
//    }
//    printf("\n");
    
    for(j=0;j<prno;j++){
        for(i=0;i<ftno;i++)         {
            prm[j][i]=cor[i][ind[j]];
        }
    }
    
    sz2=0.;
    //printf("Variances per feature:  ");
    for(j=0;j<prno;j++){
        //printf("%5.4f ",ev[j]/sz1);
        sz2+=ev[j];
    }
    
    Log("  Overall projected variances :",sz2 / sz1);
    
    free(ev);
    free(ind);
    
    return 0;
}

PCAExtractionProcessor::PCAExtractionProcessor(LFPBuffer* buffer)
: PCAExtractionProcessor(buffer,
		buffer->config_->getInt("pca.num.pc"),
		buffer->config_->getInt("pca.waveshape.samples"),
		buffer->config_->getInt("pca.min.samples"),
		buffer->config_->getBool("pca.load"),
		buffer->config_->getBool("pca.save", !buffer->config_->getBool("pca.load")),
		buffer->config_->getOutPath("pca.path.base")
		){
}

PCAExtractionProcessor::PCAExtractionProcessor(LFPBuffer *buffer, const unsigned int& num_pc,
		const unsigned int& waveshape_samples, const unsigned int& min_samples, const bool load_transform,
		const bool save_transform, const std::string& pc_path)
: LFPProcessor(buffer)
, num_pc_(num_pc)
, waveshape_samples_(waveshape_samples)
, min_samples_(min_samples)
, load_transform_(load_transform)
, save_transform_(save_transform)
, pc_path_(pc_path)
, cleanup_ws_(buffer->config_->getBool("waveshape.cleanup", false))
, feature_scale_(buffer->config_->getFloat("pca.scale", 1.0))
{
    num_spikes = new unsigned int[buffer->tetr_info_->tetrodes_number()];
    pca_done_ = new bool[buffer->tetr_info_->tetrodes_number()];
    
    for(size_t t=0; t < buffer->tetr_info_->tetrodes_number(); ++t){
        num_spikes[t] = 0;
        pca_done_[t] = false;
    }
    
    const int nchan = buffer->CHANNEL_NUM;
    
    cor_ = new int**[nchan];
    mean_ = new int*[nchan];
    sumsq_ = new int*[nchan];
    stdf_ = new float*[nchan];
    
    corf_ = new float*[waveshape_samples_];
    for (size_t w=0; w<waveshape_samples_; ++w) {
        corf_[w] = new float[waveshape_samples_];
        memset(corf_[w], 0, sizeof(float)*waveshape_samples_);
    }
    
    meanf_ = new float[waveshape_samples_];
    memset(meanf_, 0, sizeof(float)*waveshape_samples_);
    
    for (int c=0; c<nchan; ++c) {
        cor_[c] = new int*[waveshape_samples_];
        for (size_t w=0; w<waveshape_samples_; ++w) {
            cor_[c][w] = new int[waveshape_samples_];
            memset(cor_[c][w], 0, sizeof(int)*waveshape_samples_);
        }
        mean_[c] = new int[waveshape_samples_];
        memset(mean_[c], 0, sizeof(int)*waveshape_samples_);
        
        sumsq_[c] = new int[waveshape_samples_];
        memset(sumsq_[c], 0, sizeof(int)*waveshape_samples_);
        
        stdf_[c] = new float[waveshape_samples_];
    }
    
    pc_transform_ = new float**[nchan];
    for (int c=0;c<nchan; ++c) {
        pc_transform_[c] = new float*[num_pc_];
        for (size_t pc=0; pc<num_pc_; ++pc) {
            pc_transform_[c][pc] = new float[waveshape_samples_];
            memset(pc_transform_[c][pc], 0, waveshape_samples_ * sizeof(float));
        }
    }
    
    // LOAD SAVED PC TRANSFORM
    if (load_transform_){
        printf("Load PC available transforms...\n");
        
        for (size_t i=0; i < buffer->tetr_info_->tetrodes_number(); ++i) {
            for (size_t ci=0; ci < buffer->tetr_info_->channels_number(i); ++ci) {
                const unsigned int chan = buffer->tetr_info_->tetrode_channels[i][ci];
                std::string pca_path(pc_path_ + Utils::Converter::int2str(chan) + ".txt");
                if (!Utils::FS::FileExists(pca_path)){
                	pca_done_[i] = false;
                	break;
                }

                std::ifstream fpc(pca_path);
                
                for (size_t pc = 0; pc < num_pc_; ++pc) {
                    for (size_t w = 0; w < waveshape_samples_; ++w) {
                        fpc >> pc_transform_[chan][pc][w];
                    }
                }
                
                pca_done_[i] = true;
                fpc.close();
            }

            if (pca_done_[i]){
            	npcdone_ ++;
            }

            // TODO handle cases when some channel PC transforms are missing
//            npcdone_ -= buffer->tetr_info_->channels_number(i) - 1;
        }
    }

    if (npcdone_ == buffer->tetr_info_->tetrodes_number()){
    	Log("All PC transforms have been loaded");
    	buf_ptr_ptr_ = &(buffer->spike_buf_pos_unproc_);
    }
    else{
    	buffer->log_string_stream_ << npcdone_<< " PC transforms loaded out of " <<  buffer->tetr_info_->tetrodes_number() << "\n";
    	buffer->Log();
    	buf_ptr_ptr_ = &(buffer->spike_buf_pos_featext_collected_);
    }
}

void PCAExtractionProcessor::compute_pcs(Spike *spike){
    int numchan = buffer->tetr_info_->number_of_channels(spike);

    if (spike->pc == nullptr){
    	buffer->AllocateFeaturesMemory(spike);

        for (int c=0; c < numchan; ++c) {
            int chan = buffer->tetr_info_->tetrode_channels[spike->tetrode_][c];
			for (size_t pc=0; pc < num_pc_; ++pc) {
				spike->pc[c * num_pc_ + pc] = 0;
				for (size_t w=0; w < waveshape_samples_; ++w) {
					// !! STANDARDIZED OR NOT
					// spike->pc[c][pc] += spike->waveshape_final[c][w] / stdf_[chan][w] * pc_transform_[chan][w][pc];
					spike->pc[c * num_pc_ + pc] += spike->waveshape_final[c][w] * pc_transform_[chan][pc][w] / feature_scale_;
				}
				spike->num_pc_ = num_pc_;
			}
        }
        if (cleanup_ws_){
        	buffer->FreeFinalWaveshapeMemory(spike);
        }
    }
}

void saveArray(std::string path, float ** array, const unsigned int& dim1, const unsigned int& dim2){
    std::ofstream fpc(path);
    
    for (size_t i=0; i < dim1; ++i) {
        for (size_t j=0; j < dim2; ++j) {
            fpc << array[i][j] << " ";
        }
        fpc << "\n";
    }
    
    fpc.close();
}

// CURRENT POLICY: wait for sufficient amount of spikes, cluster, assign clusters for past and future spikes
void PCAExtractionProcessor::process(){
    // CURRENT LOGIC: PCA is computed once after required number of spikes has been collected,
    //  computed for all spikes and all new-coming
    
    while (*buf_ptr_ptr_ < buffer->spike_buf_no_rec){
        Spike *spike = buffer->spike_buffer_[*buf_ptr_ptr_];
        
		// DEBUG
        buffer->CheckPkgIdAndReportTime(spike->pkg_id_, "Time from after package extraction until PCA start\n", false);
        if (spike->discarded_){
            (*buf_ptr_ptr_)++;
            continue;
        }
        
        if (!pca_done_[spike->tetrode_]){
            for (int chani = 0; chani < buffer->tetr_info_->number_of_channels(spike); ++chani) {
                int chan = buffer->tetr_info_->tetrode_channels[spike->tetrode_][chani];
                
                for (size_t w=0; w < waveshape_samples_; ++w) {
                    mean_[chan][w] += int(spike->waveshape_final[chani][w] / scale_);
                    sumsq_[chan][w] += int(spike->waveshape_final[chani][w] / scale_ * spike->waveshape_final[chani][w] / scale_);
                    
                    for (size_t w2=w; w2 < waveshape_samples_; ++w2) {
                        cor_[chan][w][w2] += int(spike->waveshape_final[chani][w] * spike->waveshape_final[chani][w2] / ( scale_ * scale_ ));
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
        
        num_spikes[spike->tetrode_]++;
        (*buf_ptr_ptr_)++;
    }
    
    for (size_t tetr=0; tetr < buffer->tetr_info_->tetrodes_number(); ++tetr) {
        if ((num_spikes[tetr] >= min_samples_ || buffer->pipeline_status_ == PIPELINE_STATUS_INPUT_OVER) && !pca_done_[tetr]){

			std::stringstream ss;
			ss << "Doing PCA for tetrode " << tetr << "(" << min_samples_ << " spikes collected) " << "...\n";
			buffer->Log(ss.str());

            for (size_t ci=0; ci < buffer->tetr_info_->channels_number(tetr); ++ci) {
                int channel = buffer->tetr_info_->tetrode_channels[tetr][ci];
                
                // copy cor and mean to float arrays
                for (size_t w = 0; w < waveshape_samples_; ++w){
                    meanf_[w] = (float)mean_[channel][w];
                    for (size_t w2 = w; w2 < waveshape_samples_; ++w2){
                        corf_[w][w2] = (float)cor_[channel][w][w2];
                        corf_[w2][w] = (float)cor_[channel][w][w2];
                    }
                    
                    // STD
                    stdf_[channel][w] = (float)sumsq_[channel][w]*sumsq_[channel][w] / (float)num_spikes[tetr] - (mean_[channel][w] / (float)num_spikes[tetr] * mean_[channel][w] / (float)num_spikes[tetr]);
                }
                
                // prm - projection matrix, prm[j][i] = contribution of j-th wave feature to i-th PC
                int final_ret = final(corf_, meanf_, waveshape_samples_, num_spikes[tetr], pc_transform_[channel], num_pc_);
                
                if (final_ret != 0){
                	Log("WARNING: couldn't perform PCA for channel ", channel);
                	Log("	0-PC transform will be returned", channel);
                	Log("	Please, exclude flat channels from configuration before running PCA", channel);
                }

                // invert transform if needed (for uniform representation across channels)
                for (unsigned int pc = 0; pc < num_pc_; ++pc){
                	double sum = 0;
                	for (int i=7; i < 12; ++i){
                		sum += pc_transform_[channel][pc][i];
                	}
                	std::stringstream ss;
                	ss << "Sum of weights from 7 to 11 for channel " << channel << " and PC " << pc << " equals " << sum;
                	Log(ss.str());
                	if (sum < 0){
                		 Log("Invert weights because of negative sum...");
                		 for (size_t w = 0; w < waveshape_samples_; ++w){
                			 pc_transform_[channel][pc][w] = - pc_transform_[channel][pc][w];
                		 }
                	}
                }

                // SAVE PC transform
                if (save_transform_){
                	std::string save_path = pc_path_ + Utils::Converter::int2str(channel) + std::string(".txt");
                    saveArray(save_path, pc_transform_[channel], num_pc_, waveshape_samples_);
                    buffer->Log(std::string("Saved PC transform for tetrode ") + Utils::Converter::int2str(tetr) + " to " + save_path);
                }
            }
            
            pca_done_[tetr] = true;
            
            // get PCs for all past spikes
            for (size_t s=0; s < *buf_ptr_ptr_; ++s) {
                Spike *spike = buffer->spike_buffer_[s];
                if (spike->discarded_ || (unsigned int)spike->tetrode_ != tetr){
                    continue;
                }
                
                compute_pcs(spike);
            }

            npcdone_ ++;

            if (npcdone_ == buffer->tetr_info_->tetrodes_number()){
            	Log("PCA done on all channels !");
            	// switch pointer
            	buffer->spike_buf_pos_unproc_ = *buf_ptr_ptr_;
            	buf_ptr_ptr_ = &(buffer->spike_buf_pos_unproc_);
            }
            else{
            	std::stringstream ss;
            	ss << "PCA done in " << npcdone_ << " out of " << buffer->tetr_info_->tetrodes_number() << " tetrodes...";
            	Log(ss.str());
            }
        }
    }
}

