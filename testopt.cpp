#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) {
    int aflag = 0, fflag = 0, oflag = 0;
    char *spec = NULL;
    int index;
    int c;
    int fcount;
    char algo_type;
    int oopt = 0, popt = 0, sopt = 0, fopt = 0;

    opterr = 0;


    while ((c = getopt (argc, argv, "f:a:o:")) != -1)
    switch (c)
    {
    case 'a':
        aflag = 1;
        spec = optarg;
        printf("a spec %s\n", spec);
        sscanf(spec, "%c", &algo_type);
        break;
    case 'f':
        fflag = 1;
        spec = optarg;
        printf("f spec %s\n", spec);
        sscanf(spec, "%d", &fcount);
        break;
    case 'o':
        oflag = 1;
        printf("o spec %s\n", optarg);
        for(spec = optarg;*spec != '\0'; spec++)
        switch (*spec)
        {
        case 'O':
            oopt = 1;
            break;

        case 'P':
            popt = 1;
            break;

        case 'F':
            fopt = 1;
            break;

        case 'S':
            sopt = 1;
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


    printf ("aflag = %d, fflag = %d, oflag = %d, fcount = %d, algo_type = %c, oopt = %d, fopt = %d, popt = %d, sopt = %d\n",
            aflag, fflag, oflag, fcount, algo_type, oopt, fopt, popt, sopt);

    for (index = optind; index < argc; index++)
    printf ("Non-option argument %s\n", argv[index]);
    return 0;
}
