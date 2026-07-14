#include "getopt.h"

#include <stdio.h>
#include <string.h>

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

static const char *next_short = NULL;

static int
opt_requires_arg(const char *optstring, int opt, int *optional)
{
    const char *p = strchr(optstring, opt);
    *optional = 0;
    if (!p)
        return 0;
    if (p[1] == ':') {
        if (p[2] == ':')
            *optional = 1;
        return 1;
    }
    return 0;
}

static int
handle_long(int argc, char * const argv[], const char *arg,
            const struct option *longopts, int *longindex)
{
    const char *name = arg + 2;
    const char *value = strchr(name, '=');
    size_t name_len = value ? (size_t)(value - name) : strlen(name);
    int i;

    for (i = 0; longopts && longopts[i].name; ++i) {
        if (strlen(longopts[i].name) == name_len &&
            strncmp(longopts[i].name, name, name_len) == 0) {
            if (longindex)
                *longindex = i;

            if (longopts[i].has_arg == required_argument) {
                if (value) {
                    optarg = (char *)value + 1;
                } else if (optind + 1 < argc) {
                    optarg = argv[++optind];
                } else {
                    optopt = longopts[i].val;
                    return '?';
                }
            } else if (longopts[i].has_arg == optional_argument) {
                optarg = value ? (char *)value + 1 : NULL;
            } else {
                optarg = NULL;
            }

            optind++;
            if (longopts[i].flag) {
                *longopts[i].flag = longopts[i].val;
                return 0;
            }
            return longopts[i].val;
        }
    }

    optind++;
    return '?';
}

int
getopt_long(int argc, char * const argv[], const char *optstring,
            const struct option *longopts, int *longindex)
{
    int opt;
    int optional_arg;
    int needs_arg;

    optarg = NULL;

    if (optind == 0) {
        optind = 1;
        next_short = NULL;
    }

    if (!next_short || *next_short == '\0') {
        if (optind >= argc || !argv[optind] || argv[optind][0] != '-' || argv[optind][1] == '\0')
            return -1;
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }
        if (argv[optind][1] == '-')
            return handle_long(argc, argv, argv[optind], longopts, longindex);
        next_short = argv[optind] + 1;
    }

    opt = (unsigned char)*next_short++;
    needs_arg = opt_requires_arg(optstring, opt, &optional_arg);
    if (!strchr(optstring, opt)) {
        optopt = opt;
        if (*next_short == '\0')
            optind++;
        return '?';
    }

    if (needs_arg) {
        if (*next_short != '\0') {
            optarg = (char *)next_short;
            optind++;
            next_short = NULL;
        } else if (!optional_arg && optind + 1 < argc) {
            optarg = argv[++optind];
            optind++;
            next_short = NULL;
        } else {
            if (!optional_arg) {
                optopt = opt;
                optind++;
                next_short = NULL;
                return '?';
            }
            optarg = NULL;
            optind++;
            next_short = NULL;
        }
    } else if (*next_short == '\0') {
        optind++;
        next_short = NULL;
    }

    return opt;
}
