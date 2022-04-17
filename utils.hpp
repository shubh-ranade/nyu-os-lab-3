#ifndef UTILS_HPP
#define UTILS_HPP

#include <fstream>
#include <iostream>
#include <string>
#include "process.hpp"
#include "pager.hpp"

void print_processes(std::vector<Process*>& procs);
void print_process(Process* proc);
std::string getline_clean(std::ifstream& f);
bool get_next_instruction(std::ifstream& f, char* op, int* vpage);
void cleanup(std::vector<Process*>& procs, frame_t* frame_table, Pager* pager);

#endif
