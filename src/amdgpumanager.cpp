#include "amdgpumanager.h"

#include <algorithm>

#include "lycl/AppLyra2REv2.hpp"

NextMiner::AMDGPUManager::AMDGPUManager(NextMiner::GetWork* workSource,
                                  NextMiner::Log* log) {
    this->workSource = workSource;
    this->log = log;

    const size_t workSize = 50000;

    cl_int errorCode = CL_SUCCESS;

    cl_uint numPlatformIDs = 0;

    errorCode = clGetPlatformIDs(0, nullptr, &numPlatformIDs);
    if(errorCode != CL_SUCCESS || numPlatformIDs <= 0) {
        log->printf("Could not find number of CL platform IDs", Log::Severity::Error);
    }

    std::vector<cl_platform_id> platformIds(numPlatformIDs);
    clGetPlatformIDs(numPlatformIDs, platformIds.data(), nullptr);

    const std::string platformVendorAMD("Advanced Micro Devices");
    std::vector<lycl::device> logicalDevices;


    for(size_t i = 0; i < (size_t)numPlatformIDs; ++i) {
        // check if platform is supported (currently AMD only)
        size_t infoSize = 0;
        clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, 0, nullptr, &infoSize);
        std::string infoString(infoSize, ' ');
        clGetPlatformInfo(platformIds[i], CL_PLATFORM_VENDOR, infoSize, (void*)infoString.data(), nullptr);
        if(infoString.find(platformVendorAMD) == std::string::npos) {
            log->printf("Unsupported platform vendor + infoString", Log::Severity::Warning);
            continue;
        }

        // get devices available on this platform
        cl_uint numDeviceIDs = 0;
        errorCode = clGetDeviceIDs(platformIds[i], CL_DEVICE_TYPE_GPU, 0, nullptr, &numDeviceIDs);
        if (errorCode != CL_SUCCESS || numPlatformIDs <= 0) {
            log->printf("No devices available on platform id " + std::to_string(i), Log::Severity::Warning);
            continue;
        }

        std::vector<cl_device_id> deviceIds(numDeviceIDs);
        clGetDeviceIDs(platformIds[i], CL_DEVICE_TYPE_GPU, numDeviceIDs, deviceIds.data(), nullptr);

        cl_device_topology_amd topology;
        for (size_t j = 0; j < deviceIds.size(); ++j) {
            lycl::device clDevice;
            clDevice.clPlatformId = platformIds[i];
            clDevice.clId = deviceIds[j];
            clDevice.platformIndex = (int32_t)i;
            clDevice.binaryFormat = lycl::BF_None;
            clDevice.asmProgram = lycl::AP_None;
            clDevice.workSize = workSize;
        
            cl_int status = clGetDeviceInfo(deviceIds[j], CL_DEVICE_TOPOLOGY_AMD, 
                                            sizeof(cl_device_topology_amd), &topology, nullptr);
            if(status == CL_SUCCESS) {
                if (topology.raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
                    clDevice.pcieBusId = (int32_t)topology.pcie.bus;
                }
            } else {
                log->printf("Failed to get CL_DEVICE_TOPOLOGY_AMD info. Platform index: " + std::to_string(i),
                            Log::Severity::Warning);
                clDevice.pcieBusId = 0;
            }
                
            logicalDevices.push_back(clDevice);
        }
    }

    if(!logicalDevices.size()) {
        log->printf("Failed to find any supported devices", Log::Severity::Error);
    }
    //-----------------------------------------------------------------------------
    // sort device list based on pcieBusID -> platformID
    std::sort(logicalDevices.begin(), logicalDevices.end(), lycl::compareLogicalDevices);

    for(size_t i = 0; i < logicalDevices.size(); ++i) {
        size_t infoSize = 0;
        std::string deviceName;
        clGetDeviceInfo(logicalDevices[i].clId, CL_DEVICE_NAME, 0, NULL, &infoSize);
        deviceName.resize(infoSize);
        clGetDeviceInfo(logicalDevices[i].clId, CL_DEVICE_NAME, infoSize, (void *)deviceName.data(), NULL);
        deviceName.pop_back();
    
        std::string asmProgramName;
        lycl::getAsmProgramNameFromDeviceName(deviceName, asmProgramName);

        std::string binaryFormatName;

        #ifdef __windows
        binaryFormatName = "amdcl2";
        #elif defined __linux
        // force Vega GPUs to use ROCm by default.
        if (deviceName.find("gfx9") != std::string::npos) {
            binaryFormatName = "ROCm";
        } else {
            binaryFormatName = "amdcl2"; // AMDGPU-Pro
        }
        #else
        binaryFormatName = "none";
        #endif

        lycl::EBinaryFormat binaryFormat = lycl::BF_None;
        lycl::EAsmProgram asmProgram = lycl::AP_None; 

        asmProgram = lycl::getAsmProgramName(asmProgramName);
        binaryFormat = lycl::getBinaryFormatFromName(binaryFormatName);

        logicalDevices[i].binaryFormat = binaryFormat;
        logicalDevices[i].asmProgram = asmProgram;

        
    }



}