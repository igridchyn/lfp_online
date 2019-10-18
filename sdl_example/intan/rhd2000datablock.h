//----------------------------------------------------------------------------------
// rhd2000datablock.h
//
// Intan Technoloies RHD2000 Rhythm Interface API
// Rhd2000DataBlock Class Header File
// Version 1.4 (26 February 2014)
//
// Copyright (c) 2013-2014 Intan Technologies LLC
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any applications that
// use Intan Technologies integrated circuits, and to alter it and redistribute it
// freely.
//
// See http://www.intantech.com for documentation and product information.
//----------------------------------------------------------------------------------

#ifndef RHD2000DATABLOCK_H
#define RHD2000DATABLOCK_H

#define SAMPLES_PER_DATA_BLOCK 60
#define RHD2000_HEADER_MAGIC_NUMBER 0xc691199927021942

using namespace std;

class Rhd2000EvalBoard;

class Rhd2000DataBlock
{
public:
    Rhd2000DataBlock(int numDataStreams);

    vector<unsigned int> timeStamp;
    vector<vector<vector<int> > > amplifierData;
    vector<vector<vector<int> > > auxiliaryData;
    vector<vector<int> > boardAdcData;
    vector<int> ttlIn;
    vector<int> ttlOut;

    static unsigned int calculateDataBlockSizeInWords(int numDataStreams);
    static unsigned int getSamplesPerDataBlock();
    void fillFromUsbBuffer(unsigned char usbBuffer[], int blockIndex, int numDataStreams);
    void print(int stream) const;
    void write(ofstream &saveOut, int numDataStreams) const;

private:
    void allocateIntArray3D(vector<vector<vector<int> > > &array3D, int xSize, int ySize, int zSize);
    void allocateIntArray2D(vector<vector<int> > &array2D, int xSize, int ySize);
    void allocateIntArray1D(vector<int> &array1D, int xSize);
    void allocateUIntArray1D(vector<unsigned int> &array1D, int xSize);

    void writeWordLittleEndian(ofstream &outputStream, int dataWord) const;

    bool checkUsbHeader(unsigned char usbBuffer[], int index);
    unsigned int convertUsbTimeStamp(unsigned char usbBuffer[], int index);
    int convertUsbWord(unsigned char usbBuffer[], int index);
};

#endif // RHD2000DATABLOCK_H
