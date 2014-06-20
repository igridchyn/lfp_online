CC = gcc
CFLAGS = -std=c++0x -g
LDFLAGS = -lSDL2 -lm -lstdc++ -lmlpack -larmadillo -lpthread
	
AutocorrelogramProcessor.o: ../AutocorrelogramProcessor.cpp
	${CC} ${CFLAGS} -c ../AutocorrelogramProcessor.cpp
	
CluReaderClusteringProcessor.o: ../CluReaderClusteringProcessor.cpp
	${CC} ${CFLAGS} -c ../CluReaderClusteringProcessor.cpp
	
FetFileReaderProcessor.o: ../FetFileReaderProcessor.cpp
	${CC} ${CFLAGS} -c ../FetFileReaderProcessor.cpp
	
FrequencyPowerBandProcessor.o: ../FrequencyPowerBandProcessor.cpp
	${CC} ${CFLAGS} -c ../FrequencyPowerBandProcessor.cpp
	
GMMClusteringProcessor.o: ../GMMClusteringProcessor.cpp
	${CC} ${CFLAGS} -c ../GMMClusteringProcessor.cpp
	
KDClusteringProcessor.o: ../KDClusteringProcessor.cpp
	${CC} ${CFLAGS} -c ../KDClusteringProcessor.cpp
	
LFPPipeline.o: ../LFPPipeline.cpp
	${CC} ${CFLAGS} -c ../LFPPipeline.cpp
	
LFPProcessor.o: ../LFPProcessor.cpp
	${CC} ${CFLAGS} -c ../LFPProcessor.cpp
	
PackageExractorProcessor.o: ../PackageExractorProcessor.cpp
	${CC} ${CFLAGS} -c ../PackageExractorProcessor.cpp
	
PlaceField.o: ../PlaceField.cpp
	${CC} ${CFLAGS} -c ../PlaceField.cpp
	
PlaceFieldProcessor.o: PlaceField.o ../PlaceFieldProcessor.cpp
	${CC} ${CFLAGS} -c ../PlaceFieldProcessor.cpp
	
PositionDisplayProcessor.o: ../PositionDisplayProcessor.cpp
	${CC} ${CFLAGS} -c ../PositionDisplayProcessor.cpp
	
SDLControlInputMetaProcessor.o: ../SDLControlInputMetaProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLControlInputMetaProcessor.cpp
	
SDLPCADisplayProcessor.o: ../SDLPCADisplayProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLPCADisplayProcessor.cpp
	
SDLSignalDisplayProcessor.o: ../SDLSignalDisplayProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLSignalDisplayProcessor.cpp
	
SDLSingleWindowDisplay.o: ../SDLSingleWindowDisplay.cpp
	${CC} ${CFLAGS} -c ../SDLSingleWindowDisplay.cpp
	
SDLWaveshapeDisplayProcessor.o: ../SDLWaveshapeDisplayProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLWaveshapeDisplayProcessor.cpp
	
SlowDownProcessor.o: ../SlowDownProcessor.cpp
	${CC} ${CFLAGS} -c ../SlowDownProcessor.cpp
	
SpeedEstimationProcessor.o: ../SpeedEstimationProcessor.cpp
	${CC} ${CFLAGS} -c ../SpeedEstimationProcessor.cpp
		
SpikeAlignmentProcessor.o: ../SpikeAlignmentProcessor.cpp
	${CC} ${CFLAGS} -c ../SpikeAlignmentProcessor.cpp
		
SpikeDetectorProcessor.o: ../SpikeDetectorProcessor.cpp
	${CC} ${CFLAGS} -c ../SpikeDetectorProcessor.cpp
		
UnitTestingProcessor.o: ../UnitTestingProcessor.cpp
	${CC} ${CFLAGS} -c ../UnitTestingProcessor.cpp
		
WaveshapeReconstructionProcessor.o: ../WaveshapeReconstructionProcessor.cpp
	${CC} ${CFLAGS} -c ../WaveshapeReconstructionProcessor.cpp
		
WhlFileReaderProcessor.o: ../WhlFileReaderProcessor.cpp
	${CC} ${CFLAGS} -c ../WhlFileReaderProcessor.cpp
	
main.o: ../main.cpp
	${CC} ${CFLAGS} -c ../main.cpp
		
lfp_online: main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o 
	${CC} ${CFLAGS} -lANN main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o ${LDFLAGS} -o lfp_online
	
clean:
	rm *.o
	rm lfp_online