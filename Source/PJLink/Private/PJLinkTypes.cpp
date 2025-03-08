#include "PJLinkTypes.h"

namespace PJLinkHelpers
{
    FString PowerStatusToString(EPJLinkPowerStatus PowerStatus)
    {
        switch (PowerStatus)
        {
        case EPJLinkPowerStatus::PoweredOff:
            return TEXT("Powered Off");
        case EPJLinkPowerStatus::PoweredOn:
            return TEXT("Powered On");
        case EPJLinkPowerStatus::CoolingDown:
            return TEXT("Cooling Down");
        case EPJLinkPowerStatus::WarmingUp:
            return TEXT("Warming Up");
        case EPJLinkPowerStatus::Unknown:
        default:
            return TEXT("Unknown");
        }
    }

    FString InputSourceToString(EPJLinkInputSource InputSource)
    {
        switch (InputSource)
        {
        case EPJLinkInputSource::RGB:
            return TEXT("RGB");
        case EPJLinkInputSource::VIDEO:
            return TEXT("Video");
        case EPJLinkInputSource::DIGITAL:
            return TEXT("Digital");
        case EPJLinkInputSource::STORAGE:
            return TEXT("Storage");
        case EPJLinkInputSource::NETWORK:
            return TEXT("Network");
        case EPJLinkInputSource::Unknown:
        default:
            return TEXT("Unknown");
        }
    }

    FString CommandToString(EPJLinkCommand Command)
    {
        switch (Command)
        {
        case EPJLinkCommand::POWR: return TEXT("POWR");
        case EPJLinkCommand::INPT: return TEXT("INPT");
        case EPJLinkCommand::AVMT: return TEXT("AVMT");
        case EPJLinkCommand::ERST: return TEXT("ERST");
        case EPJLinkCommand::LAMP: return TEXT("LAMP");
        case EPJLinkCommand::INST: return TEXT("INST");
        case EPJLinkCommand::NAME: return TEXT("NAME");
        case EPJLinkCommand::INF1: return TEXT("INF1");
        case EPJLinkCommand::INF2: return TEXT("INF2");
        case EPJLinkCommand::INFO: return TEXT("INFO");
        case EPJLinkCommand::CLSS: return TEXT("CLSS");
        default: return TEXT("Unknown");
        }
    }

    FString ResponseStatusToString(EPJLinkResponseStatus Status)
    {
        switch (Status)
        {
        case EPJLinkResponseStatus::Success: return TEXT("Success");
        case EPJLinkResponseStatus::UndefinedCommand: return TEXT("UndefinedCommand");
        case EPJLinkResponseStatus::OutOfParameter: return TEXT("OutOfParameter");
        case EPJLinkResponseStatus::UnavailableTime: return TEXT("UnavailableTime");
        case EPJLinkResponseStatus::ProjectorFailure: return TEXT("ProjectorFailure");
        case EPJLinkResponseStatus::AuthenticationError: return TEXT("AuthenticationError");
        case EPJLinkResponseStatus::NoResponse: return TEXT("NoResponse");
        case EPJLinkResponseStatus::Unknown:
        default: return TEXT("Unknown");
        }
    }
}