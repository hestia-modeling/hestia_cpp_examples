#ifndef FIRST_SOC_OBSERVERS_DOORBELL_H
#define FIRST_SOC_OBSERVERS_DOORBELL_H

#include <hestia/observer/i_connection_observer.h>


class DoorbellDumper : public hestia::IConnectionObserver<hestia::IMemory::Address> {
public:
    explicit DoorbellDumper(const hestia::ObserverInit& init) : hestia::IConnectionObserver<hestia::IMemory::Address>(init) {}

    void Pushed(const hestia::IMemory::Address& doorbell, uint32_t size) noexcept override {
        printf("Pushing Doorbell: %lu\n", doorbell);
    }

    void Popped(const hestia::IMemory::Address& doorbell, uint32_t size) noexcept override {
        printf("Popping Doorbell: %lu\n", doorbell);
    }

    bool Validate() const noexcept override { return true; }
};


#endif //FIRST_SOC_OBSERVERS_DOORBELL_H
