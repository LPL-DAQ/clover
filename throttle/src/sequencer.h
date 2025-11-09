#ifndef CLOVER_SEQUENCER_H
#define CLOVER_SEQUENCER_H

#include <vector>

int sequencer_prepare(int gap, std::vector<float> bps);

int sequencer_start_trace(int sock);

#endif //CLOVER_SEQUENCER_H
