/*
 * Copyright (C) 2004 Andrew Mihal
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <list>
#include <vector>

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include "enblend.h"

using namespace std;

// Global values from command line parameters.
int Verbose = 0;
int MaximumLevels = 0;
bool OneAtATime = false;
bool Wraparound = false;
uint32 OutputWidth = 0;
uint32 OutputHeight = 0;
double StitchMismatchThreshold = 0.4;
uint16 PlanarConfig;
uint16 Photometric;

// Union bounding box.
uint32 UBBFirstX;
uint32 UBBLastX;
uint32 UBBFirstY;
uint32 UBBLastY;

// The region of interest for pyramids.
uint32 ROIFirstX;
uint32 ROILastX;
uint32 ROIFirstY;
uint32 ROILastY;

/** Print the usage information and quit. */
void printUsageAndExit() {
    cout << "==== enblend, version " << VERSION << " ====" << endl;
    cout << "Usage: enblend [options] -o OUTPUT INPUTS" << endl;
    cout << endl;
    cout << "Options:" << endl;
    cout << " -o filename       Write output to file" << endl;
    //TODO stitch mismatch avoidance is work-in-progress.
    //cout << " -t float          Stitch mismatch threshold, [0.0, 1.0]" << endl;
    cout << " -l number         Maximum number of levels to use" << endl;
    cout << " -s                Blend images one at a time" << endl;
    cout << " -w                Blend across -180/+180 boundary" << endl;
    cout << " -v                Verbose" << endl;
    cout << " -h                Print this help message" << endl;
    exit(1);
}

int main(int argc, char** argv) {

    // The name of the output file.
    char *outputFileName = NULL;

    // List of input files.
    list<char*> inputFileNameList;
    list<char*>::iterator listIterator;

    // Parse command line.
    int c;
    extern char *optarg;
    extern int optind;
    while ((c = getopt(argc, argv, "wsl:o:t:vh")) != -1) {
        switch (c) {
            case 'h': {
                printUsageAndExit();
                break;
            }
            case 'v': {
                Verbose++;
                break;
            }
            case 't': {
                StitchMismatchThreshold = strtod(optarg, NULL);
                if (StitchMismatchThreshold < 0.0
                        || StitchMismatchThreshold > 1.0) {
                    cerr << "enblend: threshold must be between "
                         << "0.0 and 1.0 inclusive."
                         << endl;
                    printUsageAndExit();
                }
                break;
            }
            case 'o': {
                if (outputFileName != NULL) {
                    cerr << "enblend: more than one output file specified."
                         << endl;
                    printUsageAndExit();
                    break;
                }
                int len = strlen(optarg) + 1;
                outputFileName = (char*)malloc(len * sizeof(char));
                if (outputFileName == NULL) {
                    cerr << endl
                         << "enblend: out of memory (in main for outputFileName)" << endl;
                    exit(1);
                }
                strncpy(outputFileName, optarg, len);
                break;
            }
            case 'l': {
                MaximumLevels = atoi(optarg);
                if (MaximumLevels < 1) {
                    cerr << "enblend: maximum levels must be 1 or more."
                         << endl;
                    printUsageAndExit();
                }
                break;
            }
            case 's': {
                OneAtATime = true;
                break;
            }
            case 'w': {
                Wraparound = true;
                break;
            }
            default: {
                printUsageAndExit();
                break;
            }
        }
    }

    // Make sure mandatory output file name parameter given.
    if (outputFileName == NULL) {
        cerr << "enblend: no output file specified." << endl;
        printUsageAndExit();
    }

    // Remaining parameters are input files.
    if (optind < argc) {
        while (optind < argc) {
            inputFileNameList.push_back(argv[optind++]);
        }
    } else {
        cerr << "enblend: no input files specified." << endl;
        printUsageAndExit();
    }

    // Create output TIFF object.
    TIFF *outputTIFF = TIFFOpen(outputFileName, "w");
    if (outputTIFF == NULL) {
        cerr << "enblend: error opening output TIFF file \""
             << outputFileName
             << "\"" << endl;
        exit(1);
    }

    // Set basic parameters for output TIFF.
    TIFFSetField(outputTIFF, TIFFTAG_ORIENTATION, 1);
    TIFFSetField(outputTIFF, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(outputTIFF, TIFFTAG_BITSPERSAMPLE, 8);

    // Give output tiff the same parameters as first input tiff.
    // Check that all input tiffs are the same size.
    listIterator = inputFileNameList.begin();
    while (listIterator != inputFileNameList.end()) {
        TIFF *inputTIFF = TIFFOpen(*listIterator, "r");

        if (inputTIFF == NULL) {
            // Error opening tiff.
            cerr << "enblend: error opening input TIFF file \""
                 << *listIterator
                 << "\"" << endl;
            exit(1);
        }

        if (listIterator == inputFileNameList.begin()) {
            // The first input tiff
            TIFFGetField(inputTIFF, TIFFTAG_PLANARCONFIG, &PlanarConfig);
            TIFFGetField(inputTIFF, TIFFTAG_PHOTOMETRIC, &Photometric);
            TIFFGetField(inputTIFF, TIFFTAG_IMAGEWIDTH, &OutputWidth);
            TIFFGetField(inputTIFF, TIFFTAG_IMAGELENGTH, &OutputHeight);

            TIFFSetField(outputTIFF, TIFFTAG_IMAGEWIDTH, OutputWidth);
            TIFFSetField(outputTIFF, TIFFTAG_IMAGELENGTH, OutputHeight);
            TIFFSetField(outputTIFF, TIFFTAG_PHOTOMETRIC, Photometric);
            TIFFSetField(outputTIFF, TIFFTAG_PLANARCONFIG, PlanarConfig);

            // We already forced this to be a 4-channel-per pixel (above), so 
            // make the fourth channel be alpha if RGB image (see tiff spec)
            // If not specified, Photoshop doesn't open up with transparent area
            if (Photometric == PHOTOMETRIC_RGB) {
                uint16 v[1];				
                v[0] = EXTRASAMPLE_ASSOCALPHA;
                TIFFSetField(outputTIFF, TIFFTAG_EXTRASAMPLES, 1, v);
            }			

            if (Verbose > 0) {
                cout << "Output image size: "
                     << OutputWidth
                     << " x "
                     << OutputHeight << endl;
            }
        }
        else {
            uint32 otherWidth;
            uint32 otherHeight;
            TIFFGetField(inputTIFF, TIFFTAG_IMAGEWIDTH, &otherWidth);
            TIFFGetField(inputTIFF, TIFFTAG_IMAGELENGTH, &otherHeight);

            if (otherWidth != OutputWidth || otherHeight != OutputHeight) {
                cerr << "enblend: input TIFF \""
                     << *listIterator
                     << "\" size "
                     << otherWidth << "x" << otherHeight
                     << " is not the same size as the first TIFF \""
                     << inputFileNameList.front()
                     << "\" size "
                     << OutputWidth << "x" << OutputHeight
                     << endl;
                exit(1);
            }
        }
            
        TIFFClose(inputTIFF);
        listIterator++;
    }

    // Scanline for copying whiteImageFile to outputTIFF.
    uint32 *line = (uint32*)malloc(OutputWidth * sizeof(uint32));
    if (line == NULL) {
        cerr << endl
             << "enblend: out of memory (in main for line)" << endl;
        exit(1);
    }

    // Create the initial white image.
    FILE *whiteImageFile = assemble(inputFileNameList, false);

    // Main blending loop
    while (!inputFileNameList.empty()) {

        // Create the black image.
        FILE *blackImageFile = assemble(inputFileNameList, OneAtATime);

        // Caluclate the union bounding box of whiteImage and blackImage.
        ubbBounds(whiteImageFile, blackImageFile);

        // Estimate memory usage for this blend step.
        if (Verbose > 0) {
            // Max usage is in mask =
            // ubbWidth * ubbHeight * MaskPixel
            // + 2 * ubbWidth * ubbHeight * uint32
            uint32 ubbPixels = (UBBLastY - UBBFirstY + 1) *
                   (UBBLastX - UBBFirstX + 1);
            long long bytes = ubbPixels * sizeof(MaskPixel)
                    + 2 * ubbPixels * sizeof(uint32);
            cout << "Estimated memory required for this blend step: "
                 << (bytes / 1000000)
                 << "MB" << endl;
        }

        // Create the blend mask.
        FILE *maskFile = createMask(whiteImageFile, blackImageFile);

        // Calculate the ROI bounds and number of levels.
        uint32 levels = roiBounds(maskFile);
        if (levels == 0) {
            // Corner case - the mask indicates that blackImage has no
            // contribution to the output.
            cerr << "enblend: some images are redundant and will not be "
                 << "blended. Try blending this image in multiple stages "
                 << "with a subset of the images in each stage." << endl;
            fclose(blackImageFile);
            fclose(maskFile);
            continue;
        }
        if (MaximumLevels > 0) {
            levels = min(levels, (uint32)MaximumLevels);
        }

        // Estimate disk usage for this blend step.
        if (Verbose > 0) {
            // whiteImageFile = OutputWidth * OutputHeight * uint32
            // blackImageFile = OutputWidth * OutputHeight * uint32
            // maskFile = ubbWidth * ubbHeight * MaskPixel
            // blackLPFile = 4/3 * roiWidth * roiHeight * LPPixel
            // maskGPFile = 4/3 * roiWidth * roiHeight * LPPixel
            long long bytes = (2 * OutputWidth * OutputHeight * sizeof(uint32))
                    + ((UBBLastY - UBBFirstY + 1) * (UBBLastX - UBBFirstX + 1) * sizeof(MaskPixel))
                    + ((ROILastY - ROIFirstY + 1) * (ROILastX - ROIFirstX + 1) * sizeof(LPPixel) * 8 / 3);
            cout << "Estimated temporary disk space required for this blend step: "
                 << (bytes / 1000000)
                 << "MB" << endl;
        }

        // Copy parts of blackImage outside of ROI into whiteImage.
        copyExcludedPixels(whiteImageFile, blackImageFile);

        // Build Laplacian pyramid from blackImage
        FILE *blackLPFile = laplacianPyramidFile(blackImageFile, levels);

        // Done with blackImage temp file. It will be deleted.
        fclose(blackImageFile);

        // Build Gaussian pyramid from mask.
        FILE *maskGPFile = gaussianPyramidFile(maskFile, levels);

        // Build Laplacian pyramid from whiteImage
        // load whiteImage from disk one row at a time.
        vector<LPPixel*> *whiteLP = laplacianPyramid(whiteImageFile, levels);

        // Blend pyramids
        blend(*whiteLP, blackLPFile, maskGPFile);

        // Done with blackLP and maskGP temp files. They will be deleted.
        fclose(blackLPFile);
        fclose(maskGPFile);

        // Collapse result back into whiteImage.
        collapsePyramid(*whiteLP);

        // Copy result into whiteImageFile using maskFile as template.
        copyROIToOutputWithMask((*whiteLP)[0], whiteImageFile, maskFile);

        // Done with maskFile temp file. It will be deleted.
        fclose(maskFile);

        // Done with whiteLP.
        for (uint32 i = 0; i < levels; i++) {
            free((*whiteLP)[i]);
        }
        delete whiteLP;

    }

    // dump output scanlines.
    if (Verbose > 0) {
        cout << "Writing output file." << endl;
    }
    rewind(whiteImageFile);
    for (uint32 i = 0; i < OutputHeight; i++) {
        readFromTmpfile(line, sizeof(uint32), OutputWidth, whiteImageFile);
        TIFFWriteScanline(outputTIFF, line, i, 8);
    }

    free(line);

    // close whiteImageFile.
    fclose(whiteImageFile);

    // close outputTIFF.
    TIFFClose(outputTIFF);

    return 0;
}
