#include "utils.hpp"

#include <sstream>

using namespace std;

const string WHITESPACE = " \n\r\t\f\v";

// Utility functions for trimming string
string trim_leading(const string& s) {
    size_t first_idx_not_whitespace = s.find_first_not_of(WHITESPACE);
    return first_idx_not_whitespace == string::npos ? "" : s.substr(first_idx_not_whitespace);
}

string trim_trailing(const string& s) {
    size_t last_idx_not_whitespace = s.find_last_not_of(WHITESPACE);
    return last_idx_not_whitespace == string::npos ? "" : s.substr(0, last_idx_not_whitespace + 1);
}

string trim(const string& s) {
    return trim_trailing(trim_leading(s));
}

// Utility functions for printing
void print_processes(vector<Process*>& procs) {
    for(int i = 0; i < procs.size(); i++) {
        cout << "\nProcess " << i << ":\n";
        int j = 0;
        for(auto vma : procs[i]->getVmas()) {
            cout << "\nVMA " << j << ':';
            cout << '\n' << vma->getStartPage() << ' ' << vma->getEndPage() << ' ' << vma->isWriteProtected() << ' ' << vma->isFileMapped();
            j++;
        }
        cout << "\n=======================================\n";
    }
}

void print_process(Process* proc) {
    cout << "\nProcess " << proc->getId() << ":\n";
    int j = 0;
    for(auto vma : proc->getVmas()) {
        cout << "\nVMA " << j << ':';
        cout << '\n' << vma->getStartPage() << ' ' << vma->getEndPage() << ' ' << vma->isWriteProtected() << ' ' << vma->isFileMapped();
        j++;
    }
    cout << "\n=======================================\n";
}

string getline_clean(ifstream& f) {
    string buffer;
    getline(f, buffer);
    buffer = trim(buffer);
    
    // empty line
    if(buffer == "" && !f.eof())
        return getline_clean(f);

    // line starts with #
    if(buffer[0] == '#' && !f.eof())
        return getline_clean(f);

    // assume well formed line otherwise
    return buffer;
}

bool get_next_instruction(ifstream& f, char* op, int* vpage) {
    string buffer = getline_clean(f);
    if(buffer == "")
        return false;

    istringstream(buffer) >> *op >> *vpage;
    return true;
}

void cleanup(vector<Process*>& procs, frame_t* frame_table, Pager* pager) {
    delete pager;
    delete [] frame_table;
    for(auto proc : procs) delete proc;
}
