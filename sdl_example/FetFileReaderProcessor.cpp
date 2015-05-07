/*
 * FetFileReaderProcessor.cpp
 *
 *  Created on: Jun 12, 2014
 *      Author: igor
 */

#include "FetFileReaderProcessor.h"
#include <boost/filesystem.hpp>
#include <armadillo>
#include <iomanip>

int nclu = 0;

unsigned int NJOBS = 12;
unsigned int JOBSIZE = 100000;
double MD_THOLD = 17.0;
double CLUST_SIZE_THOLD = 100.0;
double TIME_VAR_THOLD = 1e-17;
int NFEAT = 9;

std::vector<unsigned int> clus_;
std::vector<double> dists_;

// cluster characteristics
std::vector<arma::mat> covi, mean;
std::vector<double> cluster_sizes_;

std::vector<std::vector<arma::mat> > data_;
std::vector<std::thread*> jobs_;

void cluster_thread(unsigned int job, unsigned int start_idx){
	arma::mat mah;
	clock_t start = clock();

	unsigned int found_first_pass_ = 0;

	for (unsigned int i=0; i < JOBSIZE; ++i){
		int minclu = -1;
		double mindist = 1000000000.f;

		// first pass: close enough to small clusters (ignore big and spread in time)
		for (int cl = 0; cl < nclu; ++cl){
			// skip big clusters
			if (cluster_sizes_[cl] > CLUST_SIZE_THOLD || covi[cl](NFEAT - 1, NFEAT - 1) < TIME_VAR_THOLD)
				continue;

			mah = (data_[job][i] - mean[cl]).t() * covi[cl] * (data_[job][i] - mean[cl]);
			if (mah(0, 0) < mindist && mah(0, 0) < MD_THOLD){
				mindist = mah(0, 0);
				minclu = cl;
			}
		}

		// second pass if no cluster found: no limitations - include big clusters
		if (minclu < 0){
			for (int cl = 0; cl < nclu; ++cl){
				mah = (data_[job][i] - mean[cl]).t() * covi[cl] * (data_[job][i] - mean[cl]);
				if (mah(0, 0) < mindist){
					mindist = mah(0, 0);
					minclu = cl;
				}
			}
		}else{
			found_first_pass_ ++;
		}

		clus_[start_idx + i] = minclu;
		dists_[start_idx + i] = mindist;
	}
	std::cout << std::setprecision(3) << "Done job # " << job << " in " << (clock() - start) / (float)CLOCKS_PER_SEC << " sec\n";
	std::cout << "Assigned in first pass: " << found_first_pass_ << " / " << JOBSIZE << "\n";
}

void cluster_gaussian(){
	std::ifstream ffet("/hd1/data/processing/jc129/jc129_R2_0114_sub.fet.9");
	std::ifstream fgaussians("/hd1/data/processing/jc129/jc129_R2_0114_sub.gauss");

	fgaussians >> nclu;
	// cluster 1 is not present
	nclu --;

	// for skipping fet
	int dummy = 0;
	double clu_size_ = .0;

	// read Gaussians
	for (int cl = 0; cl < nclu; ++cl){
		// cluster number
		fgaussians >> dummy;
		fgaussians >> clu_size_;

		cluster_sizes_.push_back(clu_size_);
		covi.push_back(arma::mat(NFEAT, NFEAT));
		mean.push_back(arma::mat(NFEAT, 1));

		for (int r=0; r < NFEAT; ++r){
			for (int c=0; c < NFEAT; ++c){
				fgaussians >> covi[cl](r, c);
			}
		}

		if(cl < 300){
			std::cout << cl << "\n";
			covi[cl].print();
			std::cout << "\n";
		}

		for (int r=0; r < NFEAT; ++r){
			fgaussians >> mean[cl](r, 0);
		}

		if(cl < 300){
			mean[cl].print();
			std::cout << "\n";
		}
	}

	unsigned int n_large_clu = 0, n_large_time_var = 0;
	for (int cl=0; cl < nclu; ++cl){
		if (cluster_sizes_[cl] > CLUST_SIZE_THOLD)
			n_large_clu ++;

		if(covi[cl](NFEAT - 1, NFEAT - 1) < TIME_VAR_THOLD){
			n_large_time_var ++;
		}

	}
	std::cout << n_large_clu << " largest clusters will not be considered in the first assignment pass!\n";
	std::cout << n_large_time_var << " clusters with large time variance will not be considered in the first assignment pass!\n";

	// total number of spikes read
	int scount = 0;
	ffet >> dummy;

	clus_.resize(40000000);
	dists_.resize(40000000);
	unsigned int npoints = 0, curjob = 0;
	data_.resize(NJOBS);
	for (unsigned int j=0;j < NJOBS; ++j){
		data_[j].resize(JOBSIZE);
		for (unsigned int s=0;s < JOBSIZE; ++s){
			data_[j][s] = arma::mat(NFEAT, 1);
		}
	}

	while (!ffet.eof()){
		// read spike
		for (int f = 0; f < 4; ++f){
			ffet >> data_[curjob][npoints](f * 2, 0);
			ffet >> data_[curjob][npoints](f * 2 + 1, 0);
			ffet >> dummy;
		}
		ffet >> dummy; ffet >> dummy; ffet >> dummy; ffet >> dummy;
		ffet >> data_[curjob][npoints](NFEAT - 1, 0);

		// for current job
		npoints ++;
		// total
		scount ++;
		// if enough points, start job in new thread
		if (npoints == JOBSIZE){
			if (jobs_.size() < NJOBS){
				std::cout << "Start new job # " << curjob << " with spikes starting " << scount - JOBSIZE << "!\n";
				jobs_.push_back(new std::thread(&cluster_thread, curjob, scount - JOBSIZE));
			}else{
				// wait for job to finish
				std::cout << "Wait for job to finish!\n";
				jobs_[curjob]->join();
				std::cout << "Finished job " << curjob << "!\n";
				std::cout << "Current spike count = " << scount << "\n";
				// TODO DELETE
				std::cout << "Start new job # " << curjob << " with spikes starting " << scount - JOBSIZE << "!\n";
				jobs_[curjob] = new std::thread(&cluster_thread, curjob, scount - JOBSIZE);
			}

			curjob ++;
			if (curjob == NJOBS){
				curjob = 0;
			}

			npoints = 0;
		}

		// DEBUG
//		spike.print();
//		std::cout << "\n";
//		std::cout << minclu << "\n";
//		covi[minclu].print();
//		std::cout << "\n";
//		mean[minclu].print();
//		std::cout << "\n";

		if (!(scount % 100000)){
			std::cout << scount << " done \n";
		}

		// DEBUG
//		if (scount >= 2050000){
//			std::cout << "break\n";
//			break;
//		}
	}

	// start with last piece of data
	std::cout << "Wait for job to finish!\n";
	jobs_[curjob]->join();
	std::cout << "Finished job " << curjob << "!\n";
	std::cout << "Current spike count = " << scount << "\n";
	// TODO DELETE
	jobs_[curjob] = new std::thread(&cluster_thread, curjob, scount - npoints);
	curjob ++;

	// wait for all jobs to finish
	std::cout << "Wait for ALL threads...\n";
	for (unsigned int j=0; j < NJOBS; ++j){
		if (jobs_[j]->joinable()){
			jobs_[j]->join();
			std::cout << "Joined thread # " << j << "\n";
		}
	}
	std::cout << "Joined all threads\n";

	// write clu
	std::ofstream fclu("/hd1/data/processing/jc129/jc129_R2_0114.clu.9");
	std::ofstream fdists("dists.txt");
	fclu << nclu + 1 << "\n";
	for (int s = 0; s < scount - 1; ++s){
		fclu << clus_[s] + 2 << "\n";
		fdists << dists_[s] << "\n";
	}
	fclu.flush();
	fdists.flush();
	std::cout << "CLU written\n";

	exit(0);
}

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer)
:FetFileReaderProcessor(buffer,
		buffer->config_->getInt("spike.reader.window", 2000)
		){

}

FetFileReaderProcessor::FetFileReaderProcessor(LFPBuffer *buffer, const unsigned int window_size)
: LFPProcessor(buffer)
, WINDOW_SIZE(window_size)
, read_spk_(buffer->config_->getBool("spike.reader.spk.read", false))
, read_whl_(buffer->config_->getBool("spike.reader.whl.read", false))
, binary_(buffer->config_->getBool("spike.reader.binary", false))
, report_rate_(buffer->SAMPLING_RATE * 60 * 5)
, num_files_with_spikes_(buffer->tetr_info_->tetrodes_number())
, FET_SCALING(buffer->config_->getFloat("spike.reader.fet.scaling", 5.0))
, pos_sampling_rate_(buffer->config_->getInt("pos.sampling.rate"))
, exit_on_over_(buffer->config_->getBool("spike.reader.exit.on.over", false)){
//	cluster_gaussian();

	// number of feature files that still have spike records
	file_over_.resize(num_files_with_spikes_);

	if (buffer->config_->spike_files_.size() == 0){
		Log("ERROR: 0 spike files in the list");
		exit(LFPONLINE_ERROR_SPIKE_FILES_LIST_EMPTY);
	}

	// estimate duration of all files together
	// TODO: !!! implement for binary as well
	unsigned long total_dur = 0;
	if (!binary_){
		for (unsigned int f=0; f < buffer->config_->spike_files_.size(); ++f){
			std::string path = buffer->config_->spike_files_[f] + "fet." + Utils::NUMBERS[buffer->config_->tetrodes[0]];
			std::ifstream ffet(path);
			int d;
			while (!ffet.eof()){
				ffet >> d;
			}
			ffet.close();
			total_dur += d;
		}

		unsigned int dur_sec = total_dur / buffer->SAMPLING_RATE;
		buffer->log_string_stream_ << "Approximate duration: " << dur_sec / 60 << " min, " << dur_sec % 60 << " sec\n";
		buffer->Log();
		buffer->input_duration_ = total_dur;
	}

	for (unsigned int t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t){
		last_spikies_.push_back(new Spike(0, 0));
	}

	openNextFile();

	// find out duration from first spike file in binary case
	if (binary_){
		// TODO sum up dirations of multiple files
		const unsigned int fetn = buffer->feature_space_dims_[0];
		float *tmpf = new float[fetn + 4];
		int stime;

		while (!fet_streams_[0]->eof()){
			fet_streams_[0]->read((char*)tmpf, (fetn + 4) * sizeof(float));
			fet_streams_[0]->read((char*)&stime, sizeof(int));
		}

		fet_streams_[0]->clear();
		fet_streams_[0]->seekg(0, std::ios::beg);

		buffer->input_duration_ = stime;
		Log("WARNING: ONLY FIRST INPUT FILE DURATION IS PROVIDED");
		unsigned int dur_sec = stime / buffer->SAMPLING_RATE;
		buffer->log_string_stream_ << name() << ": Input duration: " << dur_sec / 60 << " min, " << dur_sec % 60 << " sec\n";
		buffer->Log();
	}
}

FetFileReaderProcessor::~FetFileReaderProcessor() {
	for (unsigned int i = 0; i < fet_streams_.size(); ++i) {
		fet_streams_[i]->close();
	}
}

// returns nullptr if spike time < 0
Spike* FetFileReaderProcessor::readSpikeFromFile(const unsigned int tetr){
	Spike *spike = last_spikies_[tetr]; //new Spike(0, 0);
	spike->init(0, 0);

	spike->tetrode_ = tetr;
	spike->cluster_id_ = -1;

	const int chno = buffer->tetr_info_->channels_number(tetr);

	const unsigned int fetn = buffer->feature_space_dims_[tetr];

	buffer->AllocateFeaturesMemory(spike);
	buffer->AllocateExtraFeaturePointerMemory(spike);
//	spike->pc = new float[fetn];
	spike->num_channels_ = chno;

	std::ifstream& fet_stream = *(fet_streams_[tetr]);

	if (!binary_){
		for (unsigned int fet=0; fet < fetn; ++fet) {
			fet_stream >> spike->pc[fet];
			spike->pc[fet] /= FET_SCALING; // default = 5.0
		}
	}
	else{
		fet_stream.read((char*)spike->pc, fetn * sizeof(float));
	}

	spike->num_pc_ = fetn / chno;

	if (!binary_){
		fet_stream >> spike->peak_to_valley_1_;
		fet_stream >> spike->peak_to_valley_2_;
		fet_stream >> spike->intervalley_;
		fet_stream >> spike->power_;
	}
	else{
		fet_stream.read((char*) &spike->peak_to_valley_1_, sizeof(float));
		fet_stream.read((char*) &spike->peak_to_valley_2_, sizeof(float));
		fet_stream.read((char*) &spike->intervalley_, sizeof(float));
		fet_stream.read((char*) &spike->power_, sizeof(float));
	}

	spike->num_channels_ = chno;

	if (read_spk_){
		buffer->AllocateWaveshapeMemory(spike);

		std::ifstream& spk_stream = *(spk_streams_[tetr]);
//		spike->waveshape = new int*[chno];

		for (int c=0; c < chno; ++c){
//			spike->waveshape[c] = new int[128];

			if (!binary_){
				for (int w=0; w < 128; ++w){
					spk_stream >> spike->waveshape[c][w];
				}
			}
			else{
				spk_stream.read((char*)spike->waveshape[c], 128 * sizeof(ws_type));
			}
		}
	}

	int stime;
	if (!binary_){
		fet_stream >> stime;
	}
	else{
		fet_stream.read((char*)&stime, sizeof(int));
	}

	if (fet_stream.eof()){
		num_files_with_spikes_ --;
		buffer->log_string_stream_ << "Out of spikes in tetorde " << tetr << ", " << num_files_with_spikes_ << " files left\n";
		buffer->Log();
		file_over_[tetr] = true;
	}

	// ??? what is the threshold
	if (stime < 400)
		return nullptr;

	spike->pkg_id_ = (stime >= 0 ? stime : 0) + shift_;
	spike->aligned_ = true;

	return spike;
}

void FetFileReaderProcessor::openNextFile() {
	if (current_file_ < (int)buffer->config_->spike_files_.size() - 1){
		current_file_ ++;

		int num_files_ = fet_streams_.size();
		for (int i=0; i < num_files_; ++i){
			fet_streams_[i]->close();
			delete fet_streams_[i];
			if (read_spk_){
				spk_streams_[i]->close();
				delete spk_streams_[i];
			}
			if (read_whl_){
				whl_file_->close();
				delete whl_file_;
			}

			file_over_[i] = false;
		}
		fet_streams_.clear();
		spk_streams_.clear();

		num_files_with_spikes_ = buffer->tetr_info_->tetrodes_number();

		fet_path_base_ = buffer->config_->spike_files_[current_file_];
		std::vector<int>& tetrode_numbers = buffer->config_->tetrodes;

		std::string extapp = binary_ ? "b" : "";

		int dum_ncomp;
		unsigned int min_pkg_id = std::numeric_limits<unsigned int>::max();
		for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
			std::string path = fet_path_base_ + "fet" + extapp + "." + Utils::NUMBERS[tetrode_numbers[t]];
			if (!boost::filesystem::exists(path)){
				Log(std::string("ERROR: ") + path + " doesn't exist or is not available!");
				exit(LFPONLINE_ERROR_FET_FILE_DOESNT_EXIST_OR_UNAVAILABLE);
			}

			Log(std::string("Open file: ") + path);

			fet_streams_.push_back(new std::ifstream(path, binary_ ? std::ofstream::binary : std::ofstream::in));
			if (read_spk_){
				spk_streams_.push_back(new std::ifstream(fet_path_base_ + "spk" + extapp + "." + Utils::NUMBERS[tetrode_numbers[t]], binary_ ? std::ofstream::binary : std::ofstream::in));
			}

			if (read_whl_){
				whl_file_ = new std::ifstream(fet_path_base_ + "whl");
				int pos_first_pkg_id = -1;
				(*whl_file_) >> pos_first_pkg_id;
			}

			// read number of records per spike in the beginning of the file
			if (!binary_){
				*(fet_streams_[t]) >> dum_ncomp;
			}
			Spike *tspike = readSpikeFromFile(t);
			while(tspike == nullptr && !file_over_[t]){
				tspike = readSpikeFromFile(t);
			}
			last_spikies_[t] = tspike;

			if (tspike != nullptr && tspike->pkg_id_ < min_pkg_id){
				min_pkg_id = tspike->pkg_id_;
			}
		}

		buffer->pipeline_status_ = PIPELINE_STATUS_READ_FET;

		if (current_file_ == 0 && min_pkg_id > buffer->SAMPLING_RATE){
			Log("Warning: spikes in the first files start from ", min_pkg_id);
//			shift_ = - min_pkg_id;
//			for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
//				last_spikies_[t]->pkg_id_ += shift_;
//			}
		}
	}
}

void FetFileReaderProcessor::process() {
	int last_spike_pkg_id = last_pkg_id_;

	if (num_files_with_spikes_ == 0){
		if (current_file_ < (int)buffer->config_->spike_files_.size() - 1){
			shifts_.push_back(last_pkg_id_);
			shift_ = last_pkg_id_;
			Log(std::string("Out of spikes in file ") +  buffer->config_->spike_files_[current_file_]);
			openNextFile();
		}
		else{
			if (buffer->pipeline_status_ != PIPELINE_STATUS_INPUT_OVER){
				std::stringstream ss;
				ss << "End of input files, file duration: " << last_pkg_id_ / buffer->SAMPLING_RATE / 60 << " min " << (last_pkg_id_ / buffer->SAMPLING_RATE) % 60 << " sec \n";
				Log(ss.str());

				// DEBUG
				if (buffer->pos_buf_pos_ >= 2){
					Log("Last pos pkg id: ", buffer->positions_buf_[buffer->pos_buf_pos_ - 2].pkg_id_);
				}
			}
			buffer->pipeline_status_ = PIPELINE_STATUS_INPUT_OVER;
			if (exit_on_over_){
				Log("Exit on FileReader input over");
				exit(0);
			}
			return;
		}
	}

	// PROFILING
//	if (last_pkg_id_ > 1000000){
//		std::cout << "Exit from FetFile Reader - for profiling...\n";
//		exit(0);
//	}

	// read pos from whl
//	unsigned int last_pos_pkg_id = last_pkg_id_;
	while(read_whl_ && last_pkg_id_ + WINDOW_SIZE > pos_sampling_rate_ && last_pos_pkg_id_ < last_pkg_id_ + WINDOW_SIZE - pos_sampling_rate_ && !whl_file_->eof()){
		SpatialInfo *pos_entry = buffer->positions_buf_ + buffer->pos_buf_pos_;
		(*whl_file_) >> pos_entry->x_small_LED_ >> pos_entry->y_small_LED_  >> pos_entry->x_big_LED_ >> pos_entry->y_big_LED_ >> pos_entry->pkg_id_ >> pos_entry->valid;

		buffer->pos_buf_pos_ ++;
		last_pos_pkg_id_ = pos_entry->pkg_id_;
		// TODO !!! rewind
	}

	while(last_spike_pkg_id - last_pkg_id_ < WINDOW_SIZE && num_files_with_spikes_ > 0){
		// find the earliest spike
		int earliest_spike_tetrode_ = -1;
		unsigned int earliest_spike_time_ = std::numeric_limits<unsigned int>::max();

		for (size_t t = 0; t < buffer->tetr_info_->tetrodes_number(); ++t) {
			if (file_over_[t]){
				continue;
			}

			if (last_spikies_[t]->pkg_id_ < earliest_spike_time_){
				earliest_spike_tetrode_ = t;
				earliest_spike_time_ = last_spikies_[t]->pkg_id_;
			}
		}

		Spike *bspike = buffer->spike_buffer_[buffer->spike_buf_pos];
		Spike *nspike = last_spikies_[earliest_spike_tetrode_];
		*bspike = *nspike;
		nspike->pc = nullptr;
		nspike->waveshape = nullptr;
		nspike->extra_features_ = nullptr;
		bspike->assignExtraFeaturePointers();

		// add the earliest spike to the buffer and
		// UPDATE pkg_id to inuclude the shift
		buffer->AddSpike(bspike);
		last_spike_pkg_id = earliest_spike_time_;

        // SHUFFLING - exchanges last spike with random in the buffer
//		if (last_pkg_id_ > 12000000){
//			srand(clock());
//			Spike *rspike = nullptr;
//			unsigned int rand_ind = 0;
//			while (rspike == nullptr || rspike->tetrode_ != earliest_spike_tetrode_){
//				int rnd = rand();
//				rand_ind = ((long long)rnd * (long long)buffer->spike_buf_pos) / RAND_MAX;
//				rspike = buffer->spike_buffer_[rand_ind];
//			}
//			last_spikies_[earliest_spike_tetrode_]->pkg_id_ = rspike->pkg_id_;
//			rspike->pkg_id_ = earliest_spike_time_;
//			// exchange in the buffer
//			buffer->spike_buffer_[rand_ind] = last_spikies_[earliest_spike_tetrode_];
//			buffer->spike_buffer_[buffer->spike_buf_pos - 1] = rspike;
//			last_spikies_[earliest_spike_tetrode_] = rspike;
//		}

		buffer->UpdateWindowVector(bspike);

		// DEBUG
		buffer->CheckPkgIdAndReportTime(earliest_spike_time_, "Spike 12049131 read\n", true);

		// advance with the corresponding file reading and check for the end of file [change flag + cache = number of available files]
		Spike *spike = readSpikeFromFile(earliest_spike_tetrode_);
		if (spike == nullptr)
			continue;

		last_spikies_[earliest_spike_tetrode_] = spike;

		// set coords
		// find position
		// !!! TODO: interpolate, wait for next if needed [separate processor ?]
		while(buffer->positions_buf_[buffer->pos_buf_spike_pos_].pkg_id_ < spike->pkg_id_ && buffer->pos_buf_spike_pos_ < buffer->pos_buf_pos_){
			buffer->pos_buf_spike_pos_++;
		}

		if (buffer->pos_buf_spike_pos_ > 0){
			spike->x = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].x_pos();
			spike->y = buffer->positions_buf_[buffer->pos_buf_spike_pos_ - 1].y_pos();
		}

		// TODO: buffer rewind
	}

	// for next processor - clustering
	buffer->spike_buf_pos_unproc_ = buffer->spike_buf_pos;
	last_pkg_id_ = last_spike_pkg_id;

	buffer->last_pkg_id = last_pkg_id_;

	buffer->RemoveSpikesOutsideWindow(buffer->last_pkg_id);

	if (last_spike_pkg_id - last_reported_ > report_rate_){
		buffer->log_string_stream_ << "Loaded spikes for the first " << last_reported_ / report_rate_ * 5 << " minutes of recording...\n";
		buffer->Log();
		last_reported_ = last_spike_pkg_id;
	}
}
