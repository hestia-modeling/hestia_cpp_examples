#ifndef CONSUMER_H
#define CONSUMER_H

#include <hestia/counter/counter.h>
#include <hestia/toolbox/components/consumer.h>

/**
 * Consumes numbers from it's "in" port. Will provide counters for odd, even, prime numbers
 * consumed.
 */
class NumberConsumer : public hestia::ConsumerComponent<uint32_t> {
public:
    explicit NumberConsumer(const hestia::ComponentInit& init) :
            hestia::Manageable(hestia::FrameworkType::COMPONENT, init.name),
            hestia::ConsumerComponent<uint32_t>("in", init), // My input port is named "in"
            // Counters
            m_even("even", this, m_init),
            m_odd("odd", this, m_init),
            m_prime("prime", this, m_init) {}

    /**
     * Will be called anytime that my input port has transactions available
     * and bandwidth for me to read another transaction.
     */
    void Process(uint32_t transaction) noexcept override {
        if(IsEven(transaction)) {
            ++m_even;
        } else {
            ++m_odd;
            if (IsPrime(transaction)) {
                ++m_prime;
            }
        }

    }

private:

    static bool IsEven(uint32_t num) noexcept { return num % 2 == 0; }

    static bool IsPrime(uint32_t num) noexcept {
        if (num < 2) return false;
        for(uint32_t i = 3; ( i * i ) <= num; i += 2) {
            if(num % i == 0 ) { return false; }
        }
        return true;
    }

    hestia::Counter m_even; /*!< Incremented anytime a even number is consumed >*/
    hestia::Counter m_odd; /*!< Incremented anytime a odd number is consumed >*/
    hestia::Counter m_prime; /*!< Incremented anytime a prime number is consumed >*/
};

#endif //CONSUMER_H