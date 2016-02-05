CC = gcc
CFLAGS = -std=c++0x -g -O3 -Wall
LDFLAGS = -lSDL2 -lSDL2_ttf -lm -lstdc++ -lmlpack -larmadillo -lpthread -lboost_system -lboost_filesystem
	
AutocorrelogramProcessor.o: ../AutocorrelogramProcessor.cpp ../AutocorrelogramProcessor.h UserContext.o
	${CC} ${CFLAGS} -c ../AutocorrelogramProcessor.cpp
	
BinFileReaderProcessor.o: ../BinFileReaderProcessor.cpp ../BinFileReaderProcessor.h Utils.o
	${CC} ${CFLAGS} -c ../BinFileReaderProcessor.cpp
	
CluReaderClusteringProcessor.o: ../CluReaderClusteringProcessor.cpp ../CluReaderClusteringProcessor.h
	${CC} ${CFLAGS} -c ../CluReaderClusteringProcessor.cpp
	
Config.o: ../Config.cpp ../Config.h
	${CC} ${CFLAGS} -c ../Config.cpp
	
FetFileReaderProcessor.o: ../FetFileReaderProcessor.cpp ../FetFileReaderProcessor.h
	${CC} ${CFLAGS} -c ../FetFileReaderProcessor.cpp

FetFileWriterProcessor.o: ../FetFileWriterProcessor.cpp ../FetFileWriterProcessor.h LFPProcessor.o
	${CC} ${CFLAGS} -c ../FetFileWriterProcessor.cpp
	
FiringRateEstimatorProcessor.o: ../FiringRateEstimatorProcessor.cpp ../FiringRateEstimatorProcessor.h LFPProcessor.o	
	${CC} ${CFLAGS} -c ../FiringRateEstimatorProcessor.cpp

FrequencyPowerBandProcessor.o: ../FrequencyPowerBandProcessor.cpp
	${CC} ${CFLAGS} -c ../FrequencyPowerBandProcessor.cpp
	
GMMClusteringProcessor.o: ../GMMClusteringProcessor.cpp ../GMMClusteringProcessor.h
	${CC} ${CFLAGS} -c ../GMMClusteringProcessor.cpp
	
KDClusteringProcessor.o: ../KDClusteringProcessor.cpp ../KDClusteringProcessor.h
	${CC} ${CFLAGS} -c ../KDClusteringProcessor.cpp

LFPBuffer.o: ../LFPBuffer.cpp ../LFPBuffer.h
	${CC} ${CFLAGS} -c ../LFPBuffer.cpp
	
LFPPipeline.o: ../LFPPipeline.cpp ../LFPPipeline.h
	${CC} ${CFLAGS} -c ../LFPPipeline.cpp
	
LFPProcessor.o: ../LFPProcessor.cpp ../LFPProcessor.h LFPBuffer.o ../Spike.h
	${CC} ${CFLAGS} -c ../LFPProcessor.cpp
	
LPTTriggerProcessor.o: ../LPTTriggerProcessor.cpp ../LPTTriggerProcessor.h
	${CC} ${CFLAGS} -c ../LPTTriggerProcessor.cpp
	
PackageExractorProcessor.o: ../PackageExractorProcessor.cpp ../PackageExtractorProcessor.h
	${CC} ${CFLAGS} -c ../PackageExractorProcessor.cpp
	
ParallelPipelineProcessor.o: ../ParallelPipelineProcessor.cpp ../ParallelPipelineProcessor.h
	${CC} ${CFLAGS} -c ../ParallelPipelineProcessor.cpp
	
PCAExtractionProcessor.o: ../PCAExtractionProcessor.cpp ../PCAExtractionProcessor.h
	${CC} ${CFLAGS} -c ../PCAExtractionProcessor.cpp
	
PlaceField.o: ../PlaceField.cpp ../PlaceField.h
	${CC} ${CFLAGS} -c ../PlaceField.cpp
	
PlaceFieldProcessor.o: PlaceField.o ../PlaceFieldProcessor.cpp ../PlaceFieldProcessor.h
	${CC} ${CFLAGS} -c ../PlaceFieldProcessor.cpp
	
PolygonCluster.o: ../PolygonCluster.cpp ../PolygonCluster.h
	${CC} ${CFLAGS} -c ../PolygonCluster.cpp
	
PositionDisplayProcessor.o: ../PositionDisplayProcessor.cpp ../PositionDisplayProcessor.h
	${CC} ${CFLAGS} -c ../PositionDisplayProcessor.cpp
	
SDLControlInputMetaProcessor.o: ../SDLControlInputMetaProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLControlInputMetaProcessor.cpp
	
SDLPCADisplayProcessor.o: PolygonCluster.o ../SDLPCADisplayProcessor.cpp ../SDLPCADisplayProcessor.h UserContext.o SDLSingleWindowDisplay.o
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
		
SwReaderProcessor.o: ../SwReaderProcessor.cpp ../SwReaderProcessor.h
	${CC} ${CFLAGS} -c ../SwReaderProcessor.cpp
		
TetrodesInfo.o: ../TetrodesInfo.cpp ../TetrodesInfo.h
	${CC} ${CFLAGS} -c ../TetrodesInfo.cpp
		
TransProbEstimationProcessor.o: ../TransProbEstimationProcessor.cpp ../TransProbEstimationProcessor.h
	${CC} ${CFLAGS} -c ../TransProbEstimationProcessor.cpp
		
UnitTestingProcessor.o: ../UnitTestingProcessor.cpp ../UnitTestingProcessor.h
	${CC} ${CFLAGS} -c ../UnitTestingProcessor.cpp
		
UserContext.o: ../UserContext.cpp ../UserContext.h
	${CC} ${CFLAGS} -c ../UserContext.cpp
		
Utils.o: ../Utils.cpp ../Utils.h
	${CC} ${CFLAGS} -c ../Utils.cpp
		
WaveshapeReconstructionProcessor.o: ../WaveshapeReconstructionProcessor.cpp UserContext.o
	${CC} ${CFLAGS} -c ../WaveshapeReconstructionProcessor.cpp
		
WhlFileReaderProcessor.o: ../WhlFileReaderProcessor.cpp ../WhlFileReaderProcessor.h
	${CC} ${CFLAGS} -c ../WhlFileReaderProcessor.cpp
	
main.o: ../main.cpp
	${CC} ${CFLAGS} -c ../main.cpp
		
lfp_online: main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o CluReaderClusteringProcessor.o Config.o FetFileReaderProcessor.o FetFileWriterProcessor.o FiringRateEstimatorProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PlaceField.o PCAExtractionProcessor.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o 
	${CC} ${CFLAGS} -lANN main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o Config.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FetFileWriterProcessor.o FiringRateEstimatorProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PCAExtractionProcessor.o PlaceField.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o ${LDFLAGS} -o lfp_online
	
clean:
	rm *.o
	rm lfp_online
	
kde_estimator: LFPProcessor.o ../KDE_Estimator.cpp Config.o LFPBuffer.o UserContext.o PolygonCluster.o TetrodesInfo.o Utils.o
	${CC} ${CFLAGS} -lANN ../KDE_Estimator.cpp LFPProcessor.o Config.o LFPBuffer.o UserContext.o PolygonCluster.o TetrodesInfo.o Utils.o ${LDFLAGS} -o kde_estimator