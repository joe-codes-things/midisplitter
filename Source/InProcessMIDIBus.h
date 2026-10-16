#pragma once

#include "MIDIEvent.h"
#include "PluginController.h"
#include "VoiceAssignment.h"
#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

/**
 * Lightweight in-process delivery bus used when host-managed IPC is unavailable.
 *
 * This keeps local processor and pipeline tests deterministic, and also gives
 * same-process plugin instances a fallback path while format-specific IPC is
 * being brought up.
 */
class InProcessMIDIBus {
public:
    using Callback = std::function<void(const std::vector<MIDIEvent>&, uint8_t)>;

    static size_t subscribe(PluginController::PluginFormat format, uint8_t voiceNumber, Callback callback) {
        std::lock_guard<std::mutex> lock(getMutex());

        const size_t id = getNextId()++;
        getSubscribers().push_back({ id, format, voiceNumber, std::move(callback) });
        return id;
    }

    static void unsubscribe(size_t id) {
        std::lock_guard<std::mutex> lock(getMutex());

        auto& subscribers = getSubscribers();
        subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                                         [id](const Subscriber& subscriber) {
                                             return subscriber.id == id;
                                         }),
                          subscribers.end());
    }

    static bool publish(PluginController::PluginFormat format, const std::vector<VoiceAssignment>& assignments) {
        std::vector<Delivery> deliveries;

        {
            std::lock_guard<std::mutex> lock(getMutex());

            for (const auto& assignment : assignments) {
                bool hasExactDelivery = false;

                for (const auto& subscriber : getSubscribers()) {
                    if (subscriber.format == format && subscriber.voiceNumber == assignment.voiceNumber && subscriber.callback) {
                        deliveries.push_back({ subscriber.callback, assignment.notes, static_cast<uint8_t>(assignment.voiceNumber) });
                        hasExactDelivery = true;
                    }
                }

                if (!hasExactDelivery && !assignment.notes.empty()) {
                    auto fallback = std::find_if(getSubscribers().begin(), getSubscribers().end(),
                                                 [format](const Subscriber& subscriber) {
                                                     return subscriber.format == format && subscriber.callback;
                                                 });

                    if (fallback != getSubscribers().end()) {
                        deliveries.push_back({ fallback->callback, assignment.notes, fallback->voiceNumber });
                    }
                }
            }
        }

        for (const auto& delivery : deliveries) {
            delivery.callback(delivery.events, delivery.voiceNumber);
        }

        return !deliveries.empty();
    }

private:
    struct Subscriber {
        size_t id;
        PluginController::PluginFormat format;
        uint8_t voiceNumber;
        Callback callback;
    };

    struct Delivery {
        Callback callback;
        std::vector<MIDIEvent> events;
        uint8_t voiceNumber;
    };

    static std::mutex& getMutex() {
        static std::mutex mutex;
        return mutex;
    }

    static std::vector<Subscriber>& getSubscribers() {
        static std::vector<Subscriber> subscribers;
        return subscribers;
    }

    static size_t& getNextId() {
        static size_t nextId = 1;
        return nextId;
    }
};
