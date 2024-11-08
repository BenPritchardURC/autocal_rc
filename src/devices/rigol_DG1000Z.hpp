#pragma once

namespace RIGOL_DG1000Z
{

    // NOTE:
    //		these functions in here are very specific to the way we are using
    // 		the signal generator to calibrate a rogowski coil trip unit.
    // 		I did not implement these functions to be able to deal with the
    //		function generator in a "general" kind of way.

    //      additionally, all the functions only deal with channel 1 of the
    //      function generator.

    bool OpenVISAInterface();
    void CloseVISAInterface();

    bool WriteCommand(const std::string &command);
    bool WriteCommandReadResponse(const std::string &command, std::string &response);

    bool Initialize();

    bool SetupToApplySINWave(bool use50Hz, const std::string &voltsRMSAsString);
    bool EnableOutput();
    bool DisableOutput();

    bool SetupSyncOutput();
    void ProcessSCPIScript(const std::string &filename);

}
