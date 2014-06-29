CC = gcc
CFLAGS = -std=c++0x -g
LDFLAGS = -lSDL2 -lm -lstdc++ -lmlpack -larmadillo -lpthread
	
AutocorrelogramProcessor.o: ../AutocorrelogramProcessor.cpp ../AutocorrelogramProcessor.h
	${CC} ${CFLAGS} -c ../AutocorrelogramProcessor.cpp
	
CluReaderClusteringProcessor.o: ../CluReaderClusteringProcessor.cpp ../CluReaderClusteringProcessor.h
	${CC} ${CFLAGS} -c ../CluReaderClusteringProcessor.cpp
	
FetFileReaderProcessor.o: ../FetFileReaderProcessor.cpp ../FetFileReaderProcessor.h
	${CC} ${CFLAGS} -c ../FetFileReaderProcessor.cpp
	
FrequencyPowerBandProcessor.o: ../FrequencyPowerBandProcessor.cpp
	${CC} ${CFLAGS} -c ../FrequencyPowerBandProcessor.cpp
	
GMMClusteringProcessor.o: ../GMMClusteringProcessor.cpp ../GMMClusteringProcessor.h
	${CC} ${CFLAGS} -c ../GMMClusteringProcessor.cpp
	
KDClusteringProcessor.o: ../KDClusteringProcessor.cpp ../KDClusteringProcessor.h
	${CC} ${CFLAGS} -c ../KDClusteringProcessor.cpp
	
LFPPipeline.o: ../LFPPipeline.cpp ../LFPPipeline.h
	${CC} ${CFLAGS} -c ../LFPPipeline.cpp
	
LFPProcessor.o: ../LFPProcessor.cpp ../LFPProcessor.h
	${CC} ${CFLAGS} -c ../LFPProcessor.cpp
	
PackageExractorProcessor.o: ../PackageExractorProcessor.cpp
	${CC} ${CFLAGS} -c ../PackageExractorProcessor.cpp
	
PlaceField.o: ../PlaceField.cpp ../PlaceField.h
	${CC} ${CFLAGS} -c ../PlaceField.cpp
	
PlaceFieldProcessor.o: PlaceField.o ../PlaceFieldProcessor.cpp ../PlaceFieldProcessor.h
	${CC} ${CFLAGS} -c ../PlaceFieldProcessor.cpp
	
PositionDisplayProcessor.o: ../PositionDisplayProcessor.cpp ../PositionDisplayProcessor.h
	${CC} ${CFLAGS} -c ../PositionDisplayProcessor.cpp
	
SDLControlInputMetaProcessor.o: ../SDLControlInputMetaProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLControlInputMetaProcessor.cpp
	
SDLPCADisplayProcessor.o: ../SDLPCADisplayProcessor.cpp ../SDLPCADisplayProcessor.h
	${CC} ${CFLAGS} -c ../SDLPCADisplayProcessor.cpp
	
SDLSignalDisplayProcessor.o: ../SDLSignalDisplayProcessor.cpp ../SDLSignalDisplayProcessor.h
	${CC} ${CFLAGS} -c ../SDLSignalDisplayProcessor.cpp
	
SDLSingleWindowDisplay.o: ../SDLSingleWindowDisplay.cpp
	${CC} ${CFLAGS} -c ../SDLSingleWindowDisplay.cpp
	
SDLWaveshapeDisplayProcessor.o: ../SDLWaveshapeDisplayProcessor.cpp ../SDLWaveshapeDisplayProcessor.h
	${CC} ${CFLAGS} -c ../SDLWaveshapeDisplayProcessor.cpp
	
SlowDownProcessor.o: ../SlowDownProcessor.cpp ../SlowDownProcessor.h
	${CC} ${CFLAGS} -c ../SlowDownProcessor.cpp
	
SpeedEstimationProcessor.o: ../SpeedEstimationProcessor.cpp ../SpeedEstimationProcessor.h
	${CC} ${CFLAGS} -c ../SpeedEstimationProcessor.cpp
		
SpikeAlignmentProcessor.o: ../SpikeAlignmentProcessor.cpp
	${CC} ${CFLAGS} -c ../SpikeAlignmentProcessor.cpp
		
SpikeDetectorProcessor.o: ../SpikeDetectorProcessor.cpp ../SpikeDetectorProcessor.h
	${CC} ${CFLAGS} -c ../SpikeDetectorProcessor.cpp
		
TransProbEstimationProcessor.o: ../TransProbEstimationProcessor.cpp ../TransProbEstimationProcessor.h
	${CC} ${CFLAGS} -c ../TransProbEstimationProcessor.cpp
		
UnitTestingProcessor.o: ../UnitTestingProcessor.cpp ../UnitTestingProcessor.h
	${CC} ${CFLAGS} -c ../UnitTestingProcessor.cpp
		
WaveshapeReconstructionProcessor.o: ../WaveshapeReconstructionProcessor.cpp
	${CC} ${CFLAGS} -c ../WaveshapeReconstructionProcessor.cpp
		
WhlFileReaderProcessor.o: ../WhlFileReaderProcessor.cpp ../WhlFileReaderProcessor.h
	${CC} ${CFLAGS} -c ../WhlFileReaderProcessor.cpp
	
main.o: ../main.cpp
	${CC} ${CFLAGS} -c ../main.cpp
		
lfp_online: main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o TransProbEstimationProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o 
	${CC} ${CFLAGS} -lANN main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o TransProbEstimationProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o ${LDFLAGS} -o lfp_online
	
clean:
	rm *.o
	rm lfp_online
	
kde_estimator: LFPProcessor.o ../KDE_Estimator.cpp
	${CC} ${CFLAGS} -lANN ../KDE_Estimator.cpp LFPProcessor.o ${LDFLAGS} -o kde_estimator