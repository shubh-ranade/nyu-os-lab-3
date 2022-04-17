#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <deque>
#include <sstream>
#include "pager.hpp"
#include "process.hpp"
#include "utils.hpp"

using namespace std;

// costs read/write (load/store) instructions count as 1, context_switches instructions=130, process exits instructions=1250. In addition if the following operations counts as follows:
// maps=300, unmaps=400, ins=3100, outs=2700, fins=2800, fouts=2400, zeros=140, segv=340, segprot=420
#define RW_COST 1
#define CTX_SWITCH_COST 130
#define PROC_EXIT_COST 1250
#define MAP_COST 300
#define UNMAP_COST 400
#define IN_COST 3100
#define OUT_COST 2700
#define FIN_COST 2800
#define FOUT_COST 2400
#define ZERO_COST 140
#define SEGV_COST 340
#define SEGPROT_COST 420
unsigned long instr_count = 0, ctx_switches = 0, num_proc_exits = 0;
unsigned long long total_cost = 0;

unsigned int MAX_FRAMES;
bool aopt = false;
Pager* PAGER;
deque<frame_t*> frame_pool;

frame_t* allocate_frame_from_free_list() {
    if(frame_pool.empty())
        return NULL;

    frame_t* ret = frame_pool.front();
    frame_pool.pop_front();
    return ret;
}

frame_t* get_frame(frame_t* frame_table) {
    frame_t* frame = allocate_frame_from_free_list();
    if (frame == NULL) frame = PAGER->select_victim_frame(frame_table);
    return frame;
}

void unmap(frame_t* frame, pte_t* pte_ref, Process* old_proc) {
    // pte_t* pte_ref = procs[frame->procid]->getPte(frame->vpage);
    
    pte_ref->valid = 0;
    if(pte_ref->modified) pte_ref->paged_out = 1;
    printf(" UNMAP %d:%d\n", frame->procid, frame->vpage);
    old_proc->unmaps++;
    total_cost += UNMAP_COST;

    if(pte_ref->modified) {
        if(pte_ref->file_mapped) {
            pte_ref->paged_out = 0;
            printf(" FOUT\n");
            total_cost += FOUT_COST;
            old_proc->fouts++;
        } else {
            printf(" OUT\n");
            total_cost += OUT_COST;
            old_proc->outs++;
        }
    }

    *frame = {};
}

void exit_process(Process* proc, frame_t* frame_table) {
    int pid = proc->getId();
    for(int i = 0; i < MAX_VPAGES; i++) {
        pte_t* pte = proc->getPte(i);
        if(!pte->valid) { *pte = {}; continue; }
        
        printf(" UNMAP %d:%d\n", pid, i);
        proc->unmaps++;
        total_cost += UNMAP_COST;
        if(pte->file_mapped && pte->modified) {
            pte->paged_out = 0;
            printf(" FOUT\n");
            total_cost += FOUT_COST;
            proc->fouts++;
        }

        frame_t* used_frame = frame_table + pte->frame_number;
        // reset frame, return free pool
        used_frame->pte_ref = NULL;
        frame_pool.push_back(used_frame);

        *pte = {};
    }
}

void run_simulation(ifstream& f, vector<Process*>& procs, frame_t* frame_table) {
    char operation;
    int vpage;
    Process* current_process = nullptr;

    while (get_next_instruction(f, &operation, &vpage)) {

        // cout << "executing operation " << operation << ' ' << vpage << endl;
        printf("%lu: ==> %c %d\n", instr_count, operation, vpage);
        instr_count++;

        // handle special case of “c” and “e” instruction
        if(operation == 'c') {
            current_process = procs[vpage];
            ctx_switches++;
            total_cost += CTX_SWITCH_COST;
            continue;
        }
        if(operation == 'e') {
            printf("EXIT current process %d\n", current_process->getId());
            exit_process(current_process, frame_table);
            current_process = nullptr;
            num_proc_exits++;
            total_cost += PROC_EXIT_COST;
            // TODO: unmap and return all frames used by the process to free pool
            continue;
        }

        // now the real instructions for read and write
        pte_t* pte = current_process->getPte(vpage);

        if (!pte->valid) {
            // this in reality generates the page fault exception and now you execute
            // verify this is actually a valid page in a vma if not raise error and next inst
            if(!current_process->canAccessPage(vpage)) {
                printf(" SEGV\n");
                total_cost += SEGV_COST;
                if(operation == 'r' || operation == 'w') total_cost += RW_COST;
                current_process->segv++;
                continue;
            }
            frame_t *newframe = get_frame(frame_table);
            int addr = newframe - frame_table; // pointer arithmetic
            //-> figure out if/what to do with old frame if it was mapped
            // see general outline in MM-slides under Lab3 header and writeup below
            // see whether and how to bring in the content of the access page.
            if(newframe->pte_ref != NULL) {
                unmap(newframe, newframe->pte_ref, procs[newframe->procid]);
            }
            pte->valid = 1;
            pte->frame_number = addr;
            pte->modified = 0;
            pte->referenced = 0;
            if(pte->file_mapped) {
                printf(" FIN\n");
                total_cost += FIN_COST;
                current_process->fins++;
            } else if(pte->paged_out) {
                printf(" IN\n");
                total_cost += IN_COST;
                current_process->ins++;
            } else if(!pte->paged_out) {
                printf(" ZERO\n");
                total_cost += ZERO_COST;
                current_process->zeros++;
            }
            printf(" MAP %d\n", addr);
            total_cost += MAP_COST;
            current_process->maps++;
            // create frame->vpage reverse mapping
            newframe->procid = current_process->getId();
            newframe->vpage = vpage;
            newframe->pte_ref = pte;
        }
        
        // check write protection
        if(operation == 'w') {
            pte->referenced = 1;
            total_cost += RW_COST;
            if(pte->write_protected) {
                printf(" SEGPROT\n");
                total_cost += SEGPROT_COST;
                current_process->segprot++;
            } else {
                pte->modified = 1;
            }
        }
        if(operation == 'r') {
            pte->referenced = 1;
            total_cost += RW_COST;
        }
        // simulate instruction execution by hardware by updating the R/M PTE bits
        //update_pte(read/modify) bits based on operations.
    }

    // per process output
    // page tables
    for(int i = 0; i < procs.size(); i++) {
        Process* proc = procs[i];
        printf("PT[%d]:", i);
        for(int j = 0; j < MAX_VPAGES; j++) {
            pte_t* pte = proc->getPte(j);
            
            if(!pte->valid) {
                if(pte->paged_out) printf(" #");
                else printf(" *");
                continue;
            }
            char rchar = pte->referenced ? 'R' : '-';
            char mchar = pte->modified ? 'M' : '-';
            char schar = pte->paged_out ? 'S' : '-';
            printf(" %d:%c%c%c", j, rchar, mchar, schar);
        }
        printf("\n");
    }

    // frame table
    printf("FT:");
    for(int i = 0; i < MAX_FRAMES; i++) {
        frame_t* fte = frame_table + i;
        if(!fte->pte_ref) printf(" *");
        else printf(" %d:%d", fte->procid, fte->vpage);
    }
    printf("\n");

    // proc stats
    for(int i = 0; i < procs.size(); i++) {
        Process* proc = procs[i];
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
                    proc->getId(),
                    proc->unmaps, proc->maps, proc->ins, proc->outs,
                    proc->fins, proc->fouts, proc->zeros,
                    proc->segv, proc->segprot);
    }

    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
            instr_count, ctx_switches, num_proc_exits, total_cost, sizeof(pte_t));
}

int main(int argc, char** argv) {

    // handle arguments with getopt
    int aflag = 0, fflag = 0, oflag = 0;
    char *spec = NULL;
    int index;
    int c;
    int f_arg;
    char algo_type;
    bool oopt = false, popt = false, sopt = false, fopt = false;

    opterr = 0;

    while ((c = getopt (argc, argv, "f:a:o:")) != -1)
    switch (c)
    {
    case 'a':
        aflag = 1;
        spec = optarg;
        sscanf(spec, "%c", &algo_type);
        break;
    case 'f':
        fflag = 1;
        spec = optarg;
        sscanf(spec, "%d", &f_arg);
        break;
    case 'o':
        oflag = 1;
        for(spec = optarg;*spec != '\0'; spec++)
        switch (*spec)
        {
        case 'O':
            oopt = true;
            break;

        case 'P':
            popt = true;
            break;

        case 'F':
            fopt = true;
            break;

        case 'S':
            sopt = true;
            break;
        
        case 'a':
            aopt = true;
            break;

        default:
            printf("Ignoring unknown opt %s\n", spec);
            break;
        }
        break;

    case '?':
        if (optopt == 'f' || optopt == 'a' || optopt == 'o')
            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
            fprintf (stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
        exit(EXIT_FAILURE);
    
    default:
        printf("ERROR opt %c\n", c);
        exit(EXIT_FAILURE);
    }

    if(!argv[optind] || !argv[optind+1]) {
		printf("Not enough non-optional arguments. Got %s and %s.\n", argv[optind], argv[optind+1]);
		printf("Usage: ./mmu –f<num_frames> -a<algo> [-o<options>] inputfile randomfile\n");
	}

    // cout << "f arg " << f_arg << " algo type " << algo_type << " oopt " << oopt << " sopt " << sopt;
    // cout << " popt " << popt << " fopt " << fopt << endl;

    // open inputfile
    ifstream input;
    input.open(argv[optind]);
    string buffer;

    MAX_FRAMES = f_arg;
    frame_t* frame_table = new frame_t[MAX_FRAMES]{};
    for(int i = 0; i < MAX_FRAMES; i++) // all frames are in the free pool initially
        frame_pool.push_back(frame_table + i);

    // create pager
    switch (algo_type)
    {
    case 'f':
        PAGER = new FIFOPager(false);
        break;
    
    case 'r':
        break;

    case 'c':
        PAGER = new FIFOPager(true);
        break;

    case 'e':
        PAGER = new NRUPager();
        break;

    case 'a':
        break;
    
    case 'w':
        break;
    
    default:
        printf("Unsupported replacement algorithm type %c\n", algo_type);
        printf("Exiting\n");
        exit(EXIT_FAILURE);
    }

    // read number of processes and create vector
    int n;
    buffer = getline_clean(input);
    if(buffer == "") {
        cout << "Cannot read number of processes\n";
        exit(EXIT_FAILURE);
    }
    istringstream(buffer) >> n;
    vector<Process*> procs(n);

    // read vmas for each process
    for(int i = 0; i < n; i++) {
        int vma_count;
        buffer = getline_clean(input);
        if(buffer == "") {
            cout << "Cannot read number of vmas for process " << i << '\n';
            exit(EXIT_FAILURE);
        }

        istringstream(buffer) >> vma_count;

        Process* proc = new Process(vma_count, i);
        procs[i] = proc;

        for(int j = 0; j < vma_count; j++) {
            buffer = getline_clean(input);
            if(buffer == "") {
                cout << "Cannot read vma info for vma " << j << " of process " << i << '\n';
                exit(EXIT_FAILURE);
            }

            int s, e, w, f;
            istringstream(buffer) >> s >> e >> w >> f;
            proc->setVma(j, new VMA(s, e, w, f));
        }
    }

    // TODO: insert simulation code here
    run_simulation(input, procs, frame_table);

    cleanup(procs, frame_table, PAGER);

    return EXIT_SUCCESS;
}
