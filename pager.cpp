#include "pager.hpp"
#include "process.hpp"
#include <stdio.h>

FIFOPager::FIFOPager(bool is_clock) : hand(0), second_chance(is_clock) {}
NRUPager::NRUPager()
    : last_invocation(0)
    , hand(0)
{}

frame_t* FIFOPager::select_victim_frame(frame_t* frame_table) {
    frame_t* victim;
    
    do {
        this->hand = this->hand % MAX_FRAMES;
        victim = frame_table + this->hand;
        this->hand++;
    } while(this->second_chance && victim->pte_ref->referenced && !(victim->pte_ref->referenced = 0));
    
    return victim;
}

frame_t* NRUPager::select_victim_frame(frame_t* frame_table) {
    bool reset_ref = instr_count - this->last_invocation >= NRU_WINDOW;
    if(reset_ref) this->last_invocation = instr_count;
    frame_t* victim = nullptr;
    int min_class = 100;
    this->hand = this->hand % MAX_FRAMES;
    int i = this->hand, start = this->hand;
    int num_scanned = 0;

    do {
        num_scanned++;
        pte_t* pte_ref = (frame_table + i)->pte_ref;
        int frame_class = 2 * pte_ref->referenced + pte_ref->modified;
        if(frame_class < min_class) {
            min_class = frame_class;
            victim = frame_table + this->hand;
            if(frame_class == 0 && !reset_ref) {
                this->hand++;
                break;
            }
        }
        if(reset_ref) pte_ref->referenced = 0;
        if(min_class != 0) {
            this->hand++;
            this->hand = this->hand % MAX_FRAMES;
            i = this->hand;
        } else if(min_class == 0) {
            i++;
            i = i % MAX_FRAMES;
        }
    } while(start != this->hand && start != i);

    if(min_class != 0) this->hand = (victim - frame_table + 1) % MAX_FRAMES;

    if(aopt) printf("ASELECT: hand=%d %d | %d %ld %d\n", start, reset_ref, min_class, victim - frame_table, num_scanned);
    
    return victim;
}
