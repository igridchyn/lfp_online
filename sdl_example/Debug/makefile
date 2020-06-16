CC = gcc
CFLAGS = -std=c++17 -g -O2 -Wall -Wextra -Wno-ignored-qualifiers -I/usr/local/include/SDL2 -D_REENTRANT
LDFLAGS = -lm -lstdc++ -lstdc++fs -lmlpack -larmadillo -lpthread -lboost_system -lboost_filesystem -L/usr/local/lib -Wl,-rpath,/usr/local/lib -ldl -lrt -Wl,--enable-new-dtags -lSDL2 -lSDL2_ttf -ldc1394 -lopencv_core -lopencv_highgui -lopencv_features2d -lopencv_imgproc
CLFLAGS = -Wl,--rpath=/d1/data/lfpo1/lib/GLIB -Wl,--dynamic-linker=/d1/data/lfpo1/lib/GLIB/ld-linux-x86-64.so.2 -static-libstdc++ --enable-checking
	
AutocorrelogramProcessor.o: ../AutocorrelogramProcessor.cpp ../AutocorrelogramProcessor.h UserContext.o LFPBuffer.o
	${CC} ${CFLAGS} -c ../AutocorrelogramProcessor.cpp
	
BinFileReaderProcessor.o: ../BinFileReaderProcessor.cpp ../BinFileReaderProcessor.h Utils.o LFPBuffer.o
	${CC} ${CFLAGS} -c ../BinFileReaderProcessor.cpp
	
CluReaderClusteringProcessor.o: ../CluReaderClusteringProcessor.cpp ../CluReaderClusteringProcessor.h
	${CC} ${CFLAGS} -c ../CluReaderClusteringProcessor.cpp
	
Config.o: ../Config.cpp ../Config.h
	${CC} ${CFLAGS} -c ../Config.cpp
	
FetFileReaderProcessor.o: ../FetFileReaderProcessor.cpp ../FetFileReaderProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../FetFileReaderProcessor.cpp

FetFileWriterProcessor.o: ../FetFileWriterProcessor.cpp ../FetFileWriterProcessor.h LFPProcessor.o LFPBuffer.o
	${CC} ${CFLAGS} -c ../FetFileWriterProcessor.cpp

FrequencyPowerBandProcessor.o: ../FrequencyPowerBandProcessor.cpp LFPBuffer.o
	${CC} ${CFLAGS} -c ../FrequencyPowerBandProcessor.cpp
	
GMMClusteringProcessor.o: ../GMMClusteringProcessor.cpp ../GMMClusteringProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../GMMClusteringProcessor.cpp
	
KDClusteringProcessor.o: ../KDClusteringProcessor.cpp ../KDClusteringProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../KDClusteringProcessor.cpp

LFPBuffer.o: ../LFPBuffer.cpp ../LFPBuffer.h
	${CC} ${CFLAGS} -c ../LFPBuffer.cpp
	
LFPPipeline.o: ../LFPPipeline.cpp ../LFPPipeline.h
	${CC} ${CFLAGS} -c ../LFPPipeline.cpp
	
LFPProcessor.o: ../LFPProcessor.cpp ../LFPProcessor.h LFPBuffer.o ../Spike.h
	${CC} ${CFLAGS} -c ../LFPProcessor.cpp
	
LPTTriggerProcessor.o: ../LPTTriggerProcessor.cpp ../LPTTriggerProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../LPTTriggerProcessor.cpp
	
PackageExractorProcessor.o: ../PackageExractorProcessor.cpp ../PackageExtractorProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../PackageExractorProcessor.cpp
	
ParallelPipelineProcessor.o: ../ParallelPipelineProcessor.cpp ../ParallelPipelineProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../ParallelPipelineProcessor.cpp
	
PCAExtractionProcessor.o: ../PCAExtractionProcessor.cpp ../PCAExtractionProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../PCAExtractionProcessor.cpp
	
PlaceField.o: ../PlaceField.cpp ../PlaceField.h 
	${CC} ${CFLAGS} -c ../PlaceField.cpp
	
PlaceFieldProcessor.o: PlaceField.o ../PlaceFieldProcessor.cpp ../PlaceFieldProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../PlaceFieldProcessor.cpp
	
PolygonCluster.o: ../PolygonCluster.cpp ../PolygonCluster.h
	${CC} ${CFLAGS} -c ../PolygonCluster.cpp
	
PositionDisplayProcessor.o: ../PositionDisplayProcessor.cpp ../PositionDisplayProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../PositionDisplayProcessor.cpp
	
PutativeCell.o : ../PutativeCell.cpp ../PutativeCell.h
	${CC} ${CFLAGS} -c ../PutativeCell.cpp
	
SDLControlInputMetaProcessor.o: ../SDLControlInputMetaProcessor.cpp
	${CC} ${CFLAGS} -c ../SDLControlInputMetaProcessor.cpp
	
SDLPCADisplayProcessor.o: PolygonCluster.o ../SDLPCADisplayProcessor.cpp ../SDLPCADisplayProcessor.h UserContext.o SDLSingleWindowDisplay.o LFPBuffer.o
	${CC} ${CFLAGS} -c ../SDLPCADisplayProcessor.cpp
	
SDLSignalDisplayProcessor.o: ../SDLSignalDisplayProcessor.cpp ../SDLSignalDisplayProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SDLSignalDisplayProcessor.cpp
	
SDLSingleWindowDisplay.o: ../SDLSingleWindowDisplay.cpp
	${CC} ${CFLAGS} -c ../SDLSingleWindowDisplay.cpp
	
SDLWaveshapeDisplayProcessor.o: ../SDLWaveshapeDisplayProcessor.cpp ../SDLWaveshapeDisplayProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SDLWaveshapeDisplayProcessor.cpp
	
SlowDownProcessor.o: ../SlowDownProcessor.cpp ../SlowDownProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SlowDownProcessor.cpp
	
SpeedEstimationProcessor.o: ../SpeedEstimationProcessor.cpp ../SpeedEstimationProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SpeedEstimationProcessor.cpp
		
SpikeAlignmentProcessor.o: ../SpikeAlignmentProcessor.cpp LFPBuffer.o
	${CC} ${CFLAGS} -c ../SpikeAlignmentProcessor.cpp
		
SpikeDetectorProcessor.o: ../SpikeDetectorProcessor.cpp ../SpikeDetectorProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SpikeDetectorProcessor.cpp
		
SwReaderProcessor.o: ../SwReaderProcessor.cpp ../SwReaderProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../SwReaderProcessor.cpp
		
TetrodesInfo.o: ../TetrodesInfo.cpp ../TetrodesInfo.h
	${CC} ${CFLAGS} -c ../TetrodesInfo.cpp
		
TransProbEstimationProcessor.o: ../TransProbEstimationProcessor.cpp ../TransProbEstimationProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../TransProbEstimationProcessor.cpp
		
UnitTestingProcessor.o: ../UnitTestingProcessor.cpp ../UnitTestingProcessor.h
	${CC} ${CFLAGS} -c ../UnitTestingProcessor.cpp
		
UserContext.o: ../UserContext.cpp ../UserContext.h
	${CC} ${CFLAGS} -c ../UserContext.cpp
		
Utils.o: ../Utils.cpp ../Utils.h
	${CC} ${CFLAGS} -c ../Utils.cpp
		
WaveshapeReconstructionProcessor.o: ../WaveshapeReconstructionProcessor.cpp UserContext.o LFPBuffer.o
	${CC} ${CFLAGS} -c ../WaveshapeReconstructionProcessor.cpp
		
WhlFileReaderProcessor.o: ../WhlFileReaderProcessor.cpp ../WhlFileReaderProcessor.h LFPBuffer.o
	${CC} ${CFLAGS} -c ../WhlFileReaderProcessor.cpp

okFrontPanelDLL.o: ../intan/okFrontPanelDLL.cpp ../intan/okFrontPanelDLL.h 
	${CC} ${CFLAGS} -c ../intan/okFrontPanelDLL.cpp

rhd2000registers.o: ../intan/rhd2000registers.cpp ../intan/rhd2000registers.h 
	${CC} ${CFLAGS} -c ../intan/rhd2000registers.cpp

rhd2000datablock.o: ../intan/rhd2000datablock.cpp ../intan/rhd2000datablock.h 
	${CC} ${CFLAGS} -c ../intan/rhd2000datablock.cpp

rhd2000evalboard.o: ../intan/rhd2000evalboard.cpp ../intan/rhd2000evalboard.h okFrontPanelDLL.o rhd2000datablock.o 
	${CC} ${CFLAGS} -c ../intan/rhd2000evalboard.cpp

IntanInputProcessor.o: ../IntanInputProcessor.cpp ../IntanInputProcessor.h LFPBuffer.o 
	${CC} ${CFLAGS} -c ../IntanInputProcessor.cpp

FirewireCamera.o: ../camera/FirewireCamera.cpp ../camera/FirewireCamera.h
	${CC} ${CFLAGS} -c ../camera/FirewireCamera.cpp

#PositionTrackingProcessor.o: ../PositionTrackingProcessor.cpp ../PositionTrackingProcessor.h LFPBuffer.o FirewireCamera.o
#	${CC} ${CFLAGS} -c ../PositionTrackingProcessor.cpp
	
PositionWriterProcessor.o: ../PositionWriterProcessor.cpp ../PositionWriterProcessor.h  ../AbstractWriterProcessor.h ../AbstractWriterProcessor.cpp LFPBuffer.o
	${CC} ${CFLAGS} -c ../PositionWriterProcessor.cpp
	
RawDataWriterProcessor.o: ../RawDataWriterProcessor.cpp ../RawDataWriterProcessor.h  ../AbstractWriterProcessor.h ../AbstractWriterProcessor.cpp LFPBuffer.o
	${CC} ${CFLAGS} -c ../RawDataWriterProcessor.cpp
	
main.o: ../main.cpp
	${CC} ${CFLAGS} -c ../main.cpp
		
lfp_online: main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o CluReaderClusteringProcessor.o Config.o FetFileReaderProcessor.o FetFileWriterProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PlaceField.o PCAExtractionProcessor.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o PutativeCell.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o FirewireCamera.o PositionWriterProcessor.o RawDataWriterProcessor.o IntanInputProcessor.o rhd2000evalboard.o rhd2000datablock.o rhd2000registers.o okFrontPanelDLL.o
	${CC} ${CFLAGS} -lANN main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o Config.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FetFileWriterProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PCAExtractionProcessor.o PlaceField.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o PutativeCell.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o FirewireCamera.o  PositionWriterProcessor.o RawDataWriterProcessor.o IntanInputProcessor.o rhd2000evalboard.o rhd2000datablock.o rhd2000registers.o okFrontPanelDLL.o ${LDFLAGS} -o lfp_online

lfp_online_clib: main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o CluReaderClusteringProcessor.o Config.o FetFileReaderProcessor.o FetFileWriterProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PlaceField.o PCAExtractionProcessor.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o PutativeCell.o SDLControlInputMetaProcessor.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLSingleWindowDisplay.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o IntanInputProcessor.o
	${CC} ${CFLAGS} ${CLFLAGS} -lANN main.o AutocorrelogramProcessor.o BinFileReaderProcessor.o Config.o CluReaderClusteringProcessor.o FetFileReaderProcessor.o FetFileWriterProcessor.o FrequencyPowerBandProcessor.o GMMClusteringProcessor.o KDClusteringProcessor.o LFPBuffer.o LFPPipeline.o LFPProcessor.o LPTTriggerProcessor.o PackageExractorProcessor.o ParallelPipelineProcessor.o PCAExtractionProcessor.o PlaceField.o PlaceFieldProcessor.o PolygonCluster.o PositionDisplayProcessor.o PutativeCell.o SDLPCADisplayProcessor.o SDLSignalDisplayProcessor.o SDLWaveshapeDisplayProcessor.o SlowDownProcessor.o SpeedEstimationProcessor.o SpikeAlignmentProcessor.o SpikeDetectorProcessor.o SwReaderProcessor.o TetrodesInfo.o TransProbEstimationProcessor.o UnitTestingProcessor.o UserContext.o Utils.o WaveshapeReconstructionProcessor.o WhlFileReaderProcessor.o SDLSingleWindowDisplay.o SDLControlInputMetaProcessor.o IntanInputProcessor.o ${LDFLAGS} -o lfp_online_clib

	
clean:
	rm *.o
	rm lfp_online
	
kde_estimator: LFPProcessor.o ../KDE_Estimator.cpp Config.o LFPBuffer.o UserContext.o PolygonCluster.o TetrodesInfo.o Utils.o PutativeCell.o
	${CC} ${CFLAGS} -lANN ../KDE_Estimator.cpp LFPProcessor.o Config.o LFPBuffer.o UserContext.o PolygonCluster.o PutativeCell.o TetrodesInfo.o Utils.o ${LDFLAGS} -o kde_estimator
