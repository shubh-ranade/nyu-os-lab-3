#include "pager.hpp"
#include "process.hpp"

FIFOPager::FIFOPager(bool is_clock) : hand(0), second_chance(is_clock) {}

frame_t* FIFOPager::select_victim_frame(frame_t* frame_table) {
    frame_t* ret;
    
    do {
        hand = hand % MAX_FRAMES;
        ret = frame_table + hand;
        hand++;
    } while(second_chance && ret->pte_ref->referenced && !(ret->pte_ref->referenced = 0));
    
    return ret;
}
