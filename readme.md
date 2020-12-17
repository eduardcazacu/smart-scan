# SmartScan Software Repository - RELEASE branch

## Folder Structure:
1. Root
    1. CLI - Precompiled Command Line Interface (CLI) App (64 bit). 
        1. CLI - CLI to be used with the TrakSTAR device (Make sure the driver fromNDI TrakStar is installed on the computer first).
        2. CLI-Mock - CLI working with mock data. No TrakSTAR device or drivers are needed. Only useful as a demo.
    2. Lib - This is where the magic is. Copy the SmartScanService directory to your project and include it like `#include "SmartScanService/SmartScanService.h"`. The NDI folder includes the TrakSTAR library that needs to be added to the project preferences (otherwise you will get linker errors when building). The ATC3DG64.DLL fille needs to be added next to your executable when running (i.e. for VS, in Project Root/x64/Debug/).

## Release Notes
### Working
* Connecting to the TrakStar device
* Starting/Stopping a scan at 50Hz
* Exporting the raw data as a csv file.

### Future Release
* Preferences (other sample rate etc)
* Implementing the filtering algorithm
* Calibration process (sensor and reference points)

### Known Issues
* When not all 4 sensors are connected, the service crashes trying to read the missing sensor.