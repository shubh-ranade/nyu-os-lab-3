#include "pager.hpp"
#include "process.hpp"
#include <stdio.h>

FIFOPager::FIFOPager(bool is_clock) : hand(0), second_chance(is_clock) {}
NRUPager::NRUPager()
    : last_invocation(0)
    , hand(0)
{}
AGINGPager::AGINGPager() : hand(0) {}
WSPager::WSPager() : hand(0) {}

frame_t* FIFOPager::select_victim_frame(frame_t* frame_table) {
    frame_t* victim;
    
    do {
        this->hand = this->hand % MAX_FRAMES;
        victim = frame_table + this->hand;
        this->hand++;
    } while(this->second_chance && victim->pte_ref->referenced && !(victim->pte_ref->referenced = 0));
    
    return victim;
}

frame_t* RANDOMPager::select_victim_frame(frame_t* frame_table) {
    rand_offset = rand_offset % randcount;
    frame_t* victim = frame_table + (rand_list[rand_offset] % MAX_FRAMES);
    rand_offset++;
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

frame_t* AGINGPager::select_victim_frame(frame_t* frame_table) {
    frame_t* victim = nullptr;
    unsigned int min_age = 1 << (sizeof(int) - 1), curr_age;
    this->hand = this->hand % MAX_FRAMES;
    int i = this->hand, start = this->hand;
    int prev = start == 0 ? MAX_FRAMES - 1 : start - 1;
    if(aopt) printf("ASELECT %d-%d |", start, prev);
    
    do {
        // calculate new age
        curr_age = frame_table[i].age >> 1;
        curr_age = frame_table[i].pte_ref->referenced ? curr_age | 0x80000000 : curr_age;
        frame_table[i].pte_ref->referenced = 0;
        frame_table[i].age = curr_age;
        if(aopt) printf(" %d:%x", i, curr_age);
        if(curr_age < min_age || victim == nullptr) {
            victim = frame_table + i;
            min_age = curr_age;
        }
        i++;
        i = i % MAX_FRAMES;
    } while(i != start);

    this->hand = (victim - frame_table + 1) % MAX_FRAMES;

    if(aopt) printf(" | %ld\n", victim - frame_table);

    return victim;
}

frame_t* WSPager::select_victim_frame(frame_t* frame_table) {
    int lowest = -1000;
    frame_t* victim = nullptr;
    for(int i = 0; i < MAX_FRAMES; i++) {
        frame_t* current_frame = frame_table + (this->hand + i);
        pte_t* curr_pte = current_frame->pte_ref;
        unsigned long age = instr_count - 1 - current_frame->timestamp_last_used;

        if(curr_pte->referenced) {
            curr_pte->referenced = 0;
            current_frame->timestamp_last_used = instr_count - 1;
        } else {
            if(age >= WS_TAU) {
                victim = current_frame;
                this->hand = (victim - frame_table + 1) % MAX_FRAMES;
                return victim;
            } else if (age > lowest) {
                lowest = age;
                victim = current_frame;
            }
        }
    }

    if(victim == nullptr) victim = frame_table + (this->hand);

    this->hand = (victim - frame_table + 1) % MAX_FRAMES;

    return victim;
}
