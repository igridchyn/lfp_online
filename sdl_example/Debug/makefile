CC = gcc
CFLAGS = -std=c++0x -g
LDFLAGS = -lSDL2 -lm -lstdc++ -lmlpack -larmadillo -lpthread
	
AutocorrelogramProcessor.o:
	${CC} ${CFLAGS} -c ../AutocorrelogramProcessor.cpp
	
CluReaderClusteringProcessor.o:
	${CC} ${CFLAGS} -c ../CluReaderClusteringProcessor.cpp
	
FetFileReaderProcessor.o:
	${CC} ${CFLAGS} -c ../FetFileReaderProcessor.cpp
	
FrequencyPowerBandProcessor.o:
	${CC} ${CFLAGS} -c ../FrequencyPowerBandProcessor.cpp
	
GMMClusteringProcessor.o:
	${CC} ${CFLAGS} -c ../GMMClusteringProcessor.cpp
	
LFPPipeline.o:
	${CC} ${CFLAGS} -c ../LFPPipeline.cpp
	
LFPProcessor.o:
	${CC} ${CFLAGS} -c ../LFPProcessor.cpp
	
PackageExractorProcessor.o:
	${CC} ${CFLAGS} -c ../PackageExractorProcessor.cpp
	
PlaceField.o:
	${CC} ${CFLAGS} -c ../PlaceField.cpp
	
PlaceFieldProcessor.o: PlaceField.o
	${CC} ${CFLAGS} -c ../PlaceFieldProcessor.cpp
	
PositionDisplayProcessor.o:
	${CC} ${CFLAGS} -c ../PositionDisplayProcessor.cpp
	
SDLControlInputMetaProcessor.o:
	${CC} ${CFLAGS} -c ../SDLControlInputMetaProcessor.cpp
	
SDLPCADisplayProcessor.o:
	${CC} ${CFLAGS} -c ../SDLPCADisplayProcessor.cpp
	
SDLSignalDisplayProcessor.o:
	${CC} ${CFLAGS} -c ../SDLSignalDisplayProcessor.cpp
	
SDLSingleWindowDisplay.o:
	${CC} ${CFLAGS} -c ../SDLSingleWindowDisplay.cpp
	
SDLWaveshapeDisplayProcessor.o:
	${CC} ${CFLAGS} -c ../SDLWaveshapeDisplayProcessor.cpp
	
SpeedEstimationProcessor.o:
	${CC} ${CFLAGS} -c ../SpeedEstimationProcessor.cpp
		
SpikeAlignmentProcessor.o:
	${CC} ${CFLAGS} -c ../SpikeAlignmentProcessor.cpp
		
SpikeDetectorProcessor.o:
	${CC} ${CFLAGS} -c ../SpikeDetectorProcessor.cpp
		
UnitTestingProcessor.o:
	${CC} ${CFLAGS} -c ../UnitTestingProcessor.cpp
		
WaveshapeReconstructionProcessor.o:
	${CC} ${CFLAGS} -c ../WaveshapeReconstructionProcessor.cpp
		
WhlFileReaderProcessor.o:
	${CC} ${CFLAGS} -c ../WhlFileReaderProcessor.cpp
	
main.o:
	${CC} ${CFLAGS} -c ../main.cpp
		
lfp_online: main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o 
	${CC} ${CFLAGS} main.o AutocorrelogramProcessor.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o LFPPipeline.o LFPProcessor.o PackageExractorProcessor.o PlaceField.o PlaceFieldProcessor.o PositionDisplayProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o UnitTestingProcessor.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o ${LDFLAGS} -o lfp_online
	
clean:
	rm *.o
	rm lfp_online