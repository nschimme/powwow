#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#  include <sys/param.h>
#  include <sys/wait.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <memory.h>
#include <unistd.h>
#ifdef USE_LOCALE
#  include <locale.h>
#endif
#ifdef HAVE_GETOPT_LONG
#  include <getopt.h>
#endif

#include "main.h"
#include "list.h"
#include "cmd.h"
#include "cmd2.h"
#include "edit.h"
#include "eval.h"
#include "tcp.h"
#include "tty.h"
#include "utils.h"
#include "log.h"
#include "tls.h"
#include "map.h"
#include "beam.h"

extern const char *helpname;
extern const char *copyname;
extern void mainloop(void);
extern void show_help(const char *prog);
int main(int argc, char **argv)
{
#ifdef USE_LOCALE
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr, "Failed setlocale(LC_ALL, \"\")\n");
    }
#endif

#ifdef HAVE_GETOPT_LONG
    static const struct option longopts[] = {
        { "help",    no_argument,       NULL, 'h' },
        { "tls",     optional_argument, NULL, 't' },
        { "latin1",  no_argument,       NULL, 'l' },
        { "utf8",    no_argument,       NULL, 'u' },
        { "version", no_argument,       NULL, 'v' },
        { NULL }
    };
#endif
    int cmdline_utf8 = -1;      /* --utf8: 1, --latin1: 0 */
    int nversion = 0;           /* number of --version */
    for (int c;;) {
#ifdef HAVE_GETOPT_LONG
        c = getopt_long(argc, argv, "hlt::uv", longopts, NULL);
#else
        c = getopt(argc, argv, "hltT:uv");
#endif
        if (c == -1)
            break;
        switch (c) {
        case '?':
            exit(EXIT_FAILURE);
        case 'h':
            show_help(argv[0]);
            exit(EXIT_SUCCESS);
        case 't':
#ifndef HAVE_GETOPT_LONG
            optarg = NULL;      /* only -T accepts arguments */
#endif
        case 'T': {
            const char *arg = optarg ? optarg : "";
            if (!parse_tls_flag(&hostinfo.tlsopt, &arg, false)) {
                fprintf(stderr, "invalid TLS options: '%s'\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        }
        case 'l':
            cmdline_utf8 = 0;
            break;
        case 'u':
            cmdline_utf8 = 1;
            break;
        case 'v':
            ++nversion;
            break;
        default:
            abort();
        }
    }

    if (nversion > 0) {
        printver(nversion > 1 ? 2 : 0);
        tty_flush();
        exit(EXIT_SUCCESS);
    }

    default_utf8 = cmdline_utf8 != 0;
    hostinfo.utf8 = default_utf8;

    int nargs = argc - optind;
    if (nargs > 2) {
        fprintf(stderr, "%s: too many arguments\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *p;
    int i;
    bool read_file = false;     /* run initstr ? */

    /* initializations */
    group_delim = my_strdup( "@" );

    update_now();
    ref_time = start_time = movie_last = now;

    start_clock = cpu_clock = clock();
#ifndef NO_RANDOM
    init_random((int)now.tv_sec);
#endif

    initialize_cmd();

    if ((p = getenv("POWWOWDIR"))) {
	strcpy(powwow_dir, p);
	if (powwow_dir[strlen(powwow_dir) - 1] != '/')
	    strcat(powwow_dir, "/");
    } else
	powwow_dir[0] = '\0';

    if ((p = getenv("POWWOWHELP")))
	strcpy(helpfile, p);
    else if (powwow_dir[0])
	strcpy(helpfile, powwow_dir);
    else
	strcpy(helpfile, POWWOW_DIR);

    if (helpfile[strlen(helpfile) - 1] != '/')
	strcat(helpfile, "/");
    strcat(helpfile, helpname);
    if (access(helpfile, R_OK) == -1 && !access(helpname, R_OK))
	strcpy(helpfile, helpname);

    if (powwow_dir[0])
	strcpy(copyfile, powwow_dir);
    else
	strcpy(copyfile, POWWOW_DIR);
    if (copyfile[strlen(copyfile) - 1] != '/')
	strcat(copyfile, "/");
    strcat(copyfile, copyname);
    if (access(copyfile, R_OK) == -1 && !access(copyname, R_OK))
	strcpy(copyfile, copyname);

    /* initialize variables */
    if ((var = (vars *)malloc(sizeof(vars)*(NUMTOT+max_named_vars)))) {
	for (i=0; i<NUMTOT; i++) {
	    var[i].num = &global_var[i].num;
	    var[i].str = &global_var[i].str;
	}
    } else
	syserr("malloc");

    /* stack is empty */
    paramstk.curr = 0;

    /* allocate permanent variables */
    if ((prompt = add_varnode("prompt", 1))
	&& (prompt->str = ptrnew(PARAMLEN))
	&& (marked_prompt = ptrnew(PARAMLEN))
	&& (last_line = add_varnode("last_line", 1))
	&& (last_line->str = ptrnew(PARAMLEN))
	&& (globptr[0] = ptrnew(PARAMLEN))
	&& (globptr[1] = ptrnew(PARAMLEN))
	&& !MEM_ERROR)
	;
    else
	syserr("malloc");


    /*
     ptr_bootstrap();
     utils_bootstrap();
     beam_bootstrap();
     cmd_bootstrap();
     map_bootstrap();
     eval_bootstrap();
     list_bootstrap();
     tcp_bootstrap();
     */
    edit_bootstrap();
    tty_bootstrap();

#ifdef MOTDFILE
    printmotd();
#endif

    printver(1);

    if (nargs == 0) {
	tty_printf(
"\nPowwow comes with ABSOLUTELY NO WARRANTY; for details type \"#help warranty\".\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; type \"#help copyright\" for details.\n"
		   );
    }

    if (nargs == 0 || nargs == 2) {
	tty_add_initial_binds();
	tty_add_walk_binds();
    } else if (nargs == 1 || nargs == 3) {
        /*
         * assuming first arg is definition file name
         * If three args, first is definition file name,
         * second and third are hostname and port number
         * (they overwrite the ones in definition file)
         */
        set_deffile(argv[optind++]);

        if (access(deffile, R_OK) == -1 || access(deffile, W_OK) == -1) {
	    char portnum[INTLEN];
            tty_printf("Creating %s\nHost name  :", deffile);
	    tty_flush();
	    tty_gets(hostinfo.name, BUFSIZE);
	    if (hostinfo.name[0] == '\n')
		hostinfo.name[0] = '\0';
	    else
		strtok(hostinfo.name, "\n");
            tty_puts("Port number:");
	    tty_flush();
	    tty_gets(portnum, INTLEN);
	    hostinfo.port = atoi(portnum);
            tty_add_initial_binds();
	    tty_add_walk_binds();
	    limit_mem = 1048576;
            if (save_settings() < 0)
		exit(0);
        } else if (read_settings() < 0)
	    exit(0);
	else
	    read_file = true;
    }

    /* the definition file's #host must not override --utf8/--latin1 */
    if (cmdline_utf8 >= 0)
        hostinfo.utf8 = cmdline_utf8;

    nargs = argc - optind;
    if (nargs == 2) {
        /* assume last two args are hostname and port number */
        my_strncpy(hostinfo.name, argv[optind], BUFSIZE-1);
        hostinfo.port = atoi(argv[optind + 1]);
    }

    signal_start();
    tty_start();

    tty_puts(tty_clreoscr);
    tty_putc('\n');
    tty_gotoxy(col0 = 0, lines - 2);
    tty_puts("Type #help for help.\n");
    line0 = lines - 1;

    FD_ZERO(&fdset);
    FD_SET(tty_read_fd, &fdset);

    if (*hostinfo.name)
	tcp_open("main", (*initstr ? initstr : NULL), hostinfo.name,
                 hostinfo.port, &hostinfo.tlsopt, hostinfo.utf8);

    if (read_file && !*hostinfo.name && *initstr) {
	parse_instruction(initstr, 0, 0, 1);
	history_done = 0;
    }

    confirm = 0;

    mainloop();

    /* NOTREACHED */
    return 0;
}
