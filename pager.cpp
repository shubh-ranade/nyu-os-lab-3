#include "pager.hpp"
#include "process.hpp"

FIFOPager::FIFOPager() : hand(0) {}

frame_t* FIFOPager::select_victim_frame(frame_t* frame_table) {
    hand = hand % MAX_FRAMES;
    frame_t* ret = frame_table + hand;
    hand++;
    return ret;
}
