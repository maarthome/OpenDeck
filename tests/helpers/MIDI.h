#pragma once

#include <cinttypes>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "midi/src/MIDI.h"
#include "sysex/src/SysExConf.h"
#include "system/System.h"
#include "Misc.h"

class MIDIHelper
{
    public:
    MIDIHelper() = default;

    static void sysExToUSBMIDIPacket(std::vector<uint8_t>& sysEx, std::vector<MIDI::USBMIDIpacket_t>& usbPacket)
    {
        usbPacket.clear();

        size_t inLength   = sysEx.size();
        size_t startIndex = 0;

        while (inLength > 3)
        {
            usbPacket.push_back({ MIDI::USBMIDIEvent(0, static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStartCin)), sysEx[startIndex + 0], sysEx[startIndex + 1], sysEx[startIndex + 2] });
            startIndex += 3;
            inLength -= 3;
        }

        if (inLength == 3)
        {
            usbPacket.push_back({ MIDI::USBMIDIEvent(0, static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStop3byteCin)), sysEx[startIndex + 0], sysEx[startIndex + 1], sysEx[startIndex + 2] });
        }
        else if (inLength == 2)
        {
            usbPacket.push_back({ MIDI::USBMIDIEvent(0, static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStop2byteCin)), sysEx[startIndex + 0], sysEx[startIndex + 1], 0 });
        }
        else if (inLength == 1)
        {
            usbPacket.push_back({ MIDI::USBMIDIEvent(0, static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStop1byteCin)), sysEx[startIndex + 0], 0, 0 });
        }
    }

    static bool parseUSBSysEx(std::vector<MIDI::USBMIDIpacket_t>& usbPacket, std::vector<uint8_t>& sysEx)
    {
        if (!usbPacket.size())
            return false;

        sysEx.clear();

        while (usbPacket.size())
        {
            uint8_t midiMessage = usbPacket.at(0).Event << 4;

            switch (midiMessage)
            {
            //1 byte messages
            case static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysCommon1byteCin):
            {
                //end of sysex
                sysEx.push_back(usbPacket.at(0).Data1);
                return true;
            }
            break;

            //sysex
            case static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStartCin):
            {
                //the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
                if (usbPacket.at(0).Data1 == 0xF0)
                    sysEx.clear();

                sysEx.push_back(usbPacket.at(0).Data1);
                sysEx.push_back(usbPacket.at(0).Data2);
                sysEx.push_back(usbPacket.at(0).Data3);
            }
            break;

            case static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStop2byteCin):
            {
                sysEx.push_back(usbPacket.at(0).Data1);
                sysEx.push_back(usbPacket.at(0).Data2);
                return true;
            }
            break;

            case static_cast<uint8_t>(MIDI::usbMIDIsystemCin_t::sysExStop3byteCin):
            {
                if (usbPacket.at(0).Data1 == 0xF0)
                    sysEx.clear();    //sysex message with 1 byte of payload

                sysEx.push_back(usbPacket.at(0).Data1);
                sysEx.push_back(usbPacket.at(0).Data2);
                sysEx.push_back(usbPacket.at(0).Data3);
                return true;
            }
            break;

            default:
                break;
            }

            usbPacket.erase(usbPacket.begin());
        }

        return false;
    }

    template<typename T>
    static bool setSingleSysExReq(T section, size_t index, uint16_t value)
    {
        auto             blockIndex = block(section);
        MIDI::Split14bit indexSplit;
        MIDI::Split14bit valueSplit;

        indexSplit.split(index);
        valueSplit.split(value);

        const std::vector<uint8_t> requestUint8 = {
            0xF0,
            SYSEX_MANUFACTURER_ID_0,
            SYSEX_MANUFACTURER_ID_1,
            SYSEX_MANUFACTURER_ID_2,
            static_cast<uint8_t>(SysExConf::status_t::request),
            0,
            static_cast<uint8_t>(SysExConf::wish_t::set),
            static_cast<uint8_t>(SysExConf::amount_t::single),
            static_cast<uint8_t>(blockIndex),
            static_cast<uint8_t>(section),
            indexSplit.high(),
            indexSplit.low(),
            valueSplit.high(),
            valueSplit.low(),
            0xF7
        };

        return sendRequest(requestUint8, SysExConf::wish_t::set);
    }

    template<typename T>
    static uint16_t getSingleSysExReq(T section, size_t index)
    {
        auto             blockIndex = block(section);
        MIDI::Split14bit split14bit;

        split14bit.split(index);

        const std::vector<uint8_t> requestUint8 = {
            0xF0,
            SYSEX_MANUFACTURER_ID_0,
            SYSEX_MANUFACTURER_ID_1,
            SYSEX_MANUFACTURER_ID_2,
            static_cast<uint8_t>(SysExConf::status_t::request),
            0,
            static_cast<uint8_t>(SysExConf::wish_t::get),
            static_cast<uint8_t>(SysExConf::amount_t::single),
            static_cast<uint8_t>(blockIndex),
            static_cast<uint8_t>(section),
            split14bit.high(),
            split14bit.low(),
            0,
            0,
            0xF7
        };

        return sendRequest(requestUint8, SysExConf::wish_t::get);
    }

    static std::string sendRawSysEx(std::string req)
    {
        std::string response;
        std::string lastResponseFileLocation = "/tmp/midi_in_data.txt";

        test::wsystem("rm -f " + lastResponseFileLocation, response);
        std::string cmd = std::string("stdbuf -i0 -o0 -e0 amidi -p $(amidi -l | grep -E 'OpenDeck'") + std::string(" | grep -Eo 'hw:\\S*') -S '") + req + "' -d | stdbuf -i0 -o0 -e0 tr -d '\n' > " + lastResponseFileLocation + " &";
        test::wsystem(cmd, response);

        size_t responseRetryCounter = 0;

        //once F7 appears in file, response is received
        //allow 100ms of total response time
        while (test::wsystem("grep -q 'F7' " + lastResponseFileLocation, response))
        {
            test::wsystem("sleep 0.01", response);
            responseRetryCounter++;

            if (responseRetryCounter == 10)
                break;
        }

        test::wsystem("killall amidi > /dev/null 2>&1", response);

        return lastResponse(lastResponseFileLocation);
    }

    private:
    static std::string lastResponse(const std::string& location)
    {
        //last response is in last line in provided file path
        std::ifstream file;
        std::string   lastline = "";

        file.open(location);

        if (file.is_open())
        {
            char ch = ' ';
            file.seekg(0, std::ios_base::end);

            while (ch != '\n')
            {
                file.seekg(-2, std::ios_base::cur);

                if ((int)file.tellg() <= 0)
                {                     //If passed the start of the file,
                    file.seekg(0);    //this is the start of the line
                    break;
                }

                file.get(ch);
            }

            std::getline(file, lastline);
            file.close();
        }

        return lastline;
    }

    static uint16_t sendRequest(const std::vector<uint8_t>& requestUint8, SysExConf::wish_t wish)
    {
        //convert uint8_t vector to string so it can be passed as command line argument
        std::stringstream requestString;
        requestString << std::hex << std::setfill('0') << std::uppercase;

        auto first = std::begin(requestUint8);
        auto last  = std::end(requestUint8);

        while (first != last)
        {
            requestString << std::setw(2) << static_cast<int>(*first++);

            if (first != last)
                requestString << " ";
        }

        std::string responseString = sendRawSysEx(requestString.str());

        std::cout << "req: " << requestString.str() << std::endl;
        std::cout << "res: " << responseString << std::endl;

        //convert response back to uint8 vector
        std::vector<uint8_t> responseUint8;

        for (size_t i = 0; i < responseString.length(); i += 3)
        {
            std::string byteString = responseString.substr(i, 2);
            char        byte       = (char)strtol(byteString.c_str(), NULL, 16);
            responseUint8.push_back(byte);
        }

        if (wish == SysExConf::wish_t::get)
        {
            //last two bytes are result
            MIDI::Merge14bit merge14bit;
            merge14bit.merge(responseUint8.at(responseUint8.size() - 3), responseUint8.at(responseUint8.size() - 2));
            return merge14bit.value();
        }
        else
        {
            //read status byte
            return responseUint8.at(4);
        }
    }

    static System::block_t block(System::Section::global_t section)
    {
        return System::block_t::global;
    }

    static System::block_t block(System::Section::button_t section)
    {
        return System::block_t::buttons;
    }

    static System::block_t block(System::Section::encoder_t section)
    {
        return System::block_t::encoders;
    }

    static System::block_t block(System::Section::analog_t section)
    {
        return System::block_t::analog;
    }

    static System::block_t block(System::Section::leds_t section)
    {
        return System::block_t::leds;
    }

    static System::block_t block(System::Section::display_t section)
    {
        return System::block_t::display;
    }

    static System::block_t block(System::Section::touchscreen_t section)
    {
        return System::block_t::touchscreen;
    }
};