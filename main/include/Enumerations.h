#pragma once

namespace CLASSICDIY
{
    enum NetworkSelection
    {
        NotConnected,
        APMode,
        WiFiMode,
        EthernetMode,
        ModemMode
    };

    enum NetworkState
    {
        Boot,
        ApState,
        Connecting,
        NoNetwork,
        OnLine,
        OffLine
    };

    enum ModbusMode
    {
        TCP,
        RTU
    };

    enum Mode
    {
        undefined,
        off,
        heat
    };

} // namespace CLASSICDIY