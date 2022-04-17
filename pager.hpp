#ifndef PAGER_HPP
#define PAGER_HPP

#include "pte.hpp"

class Pager {
public:
    virtual frame_t* select_victim_frame(frame_t*) = 0; // virtual base class
};

class FIFOPager : public Pager {
private:
    int hand;
public:
    FIFOPager();
    frame_t* select_victim_frame(frame_t* frame_table);
};

#endif
