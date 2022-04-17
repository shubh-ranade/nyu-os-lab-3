#ifndef PAGER_HPP
#define PAGER_HPP

#include "pte.hpp"

#define NRU_WINDOW 50
#define NUM_NRU_CLASSES 4

extern unsigned long instr_count;
extern unsigned int MAX_FRAMES;
extern bool aopt;

class Pager {
public:
    virtual frame_t* select_victim_frame(frame_t*) = 0; // virtual base class
};

class FIFOPager : public Pager {
private:
    int hand;
    bool second_chance;
public:
    FIFOPager(bool is_clock);
    frame_t* select_victim_frame(frame_t* frame_table);
};

class NRUPager : public Pager {
private:
    unsigned long last_invocation;
    int hand;
public:
    NRUPager();
    frame_t* select_victim_frame(frame_t* frame_table);
};

class AGINGPager : public Pager {
private:
    int hand;
public:
    AGINGPager();
    frame_t* select_victim_frame(frame_t* frame_table);
};

#endif
