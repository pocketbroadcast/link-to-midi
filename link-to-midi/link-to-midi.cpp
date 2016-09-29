// link-to-midi.cpp : Defines the entry point for the console application.
//

#include <memory>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <string>

#include <ableton/Link.hpp>
#include <RtMidi.h>

namespace detail {
    auto connectMidiOut(int portNumber) {
        auto out = std::make_unique<RtMidiOut>();
        out->openPort(portNumber, out->getPortName(portNumber));

        return out;
    }
}

auto connectAllMidiPorts() {
    auto midiPorts = std::vector<std::unique_ptr<RtMidiOut>>{};

    auto out = std::make_unique<RtMidiOut>();
    for (auto i = 0U; i < out->getPortCount(); i++)
        midiPorts.push_back(detail::connectMidiOut(i));

    return midiPorts;
}

void printStatus(int nrPeers, int bpm) {
    std::cout << "Ableton Link: peers " << nrPeers << " bpm: " << bpm;

    std::cout << "   \r" << std::flush;
    std::cout.fill(' ');
}

int main(int, char** argv)
{
    std::atomic<int> bpm = 120;
    std::atomic<int> nrPeers = 0;
    std::atomic<bool> run = true;

	std::cout << "Welcome to " << argv[0] << std::endl;
	std::cout << "Press any key to quit" << std::endl;
	
    const auto midiPorts = connectAllMidiPorts();
    std::cout << "Midi: peers: " << midiPorts.size() << std::endl;

    auto midiThread = std::thread([&]() {
        auto message = std::vector<unsigned char>{};
        message.push_back(0xF8);
        
        while (run) {
            for (auto& port : midiPorts)
                port->sendMessage(&message);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000 / (24 * bpm)));
        }
    });

    printStatus(nrPeers, bpm);

    ableton::Link link(bpm);
    link.setNumPeersCallback([&](const size_t peers) { nrPeers = peers; printStatus(nrPeers, bpm); });
    link.setTempoCallback([&](const double tempo) { bpm = tempo; printStatus(nrPeers, bpm); });
    link.enable(true);

    std::cin.get();
    run = false;

    midiThread.join();

    return 0;
}
