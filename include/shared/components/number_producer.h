#ifndef PRODUCER_H
#define PRODUCER_H

#include <hestia/toolbox/components/producer.h>

#include <random>

/**
 * Simple random number generator producer. Can be configured at run time to
 * generate random numbers between a range as well as the amount of numbers
 * to produce.
 */
class NumberProducer : public hestia::ProducerComponent<uint32_t> {
public:
    explicit NumberProducer(const hestia::ComponentInit &init) :
            Manageable(hestia::FrameworkType::COMPONENT, init.name),
            hestia::ProducerComponent<uint32_t>("out", init), // My output port is named "out"
            // Parameters
            m_num_transactions_remaining(GetUintParam("num_transactions")),
            m_number_generator(GetUintParam("min_number"),
                               GetUintParam("max_number")) {}

    /**
     * Function to let the base class know that I still have work to generate.
     */
    [[nodiscard]] bool HasWork() const noexcept override {
        return m_num_transactions_remaining > 0;
    }

    /**
     * Will be called anytime that my output port has room and bandwidth for
     * me to produce another transaction. In this case the transaction will
     * be a random number in the range of the min / max parameters defined
     * at run time.
     */
    uint32_t Produce() noexcept override {
        --m_num_transactions_remaining;
        return m_number_generator(m_rng);
    }

private:
    uint64_t m_num_transactions_remaining;

    // Output Generators
    std::mt19937 m_rng{{}};
    std::uniform_int_distribution<std::mt19937::result_type> m_number_generator;

};

#endif //PRODUCER_H