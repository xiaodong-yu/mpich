/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMI_PROXY_H_INCLUDED
#define PMI_PROXY_H_INCLUDED

#include "hydra_base.h"
#include "hydra_utils.h"
#include "pm_utils.h"

struct HYD_pmcd_pmip {
    struct HYD_user_global user_global;

    struct {
        int stdin_valid;
        int global_core_count;

        /* PMI */
        char *pmi_port;
        int pmi_id;             /* If this is -1, we auto-generate it */
    } system_global;            /* Global system parameters */

    struct {
        /* Upstream server contact information */
        char *server_name;
        int server_port;
        int control;
    } upstream;

    /* Currently our downstream only consists of actual MPI
     * processes */
    struct {
        int *out;
        int *err;
        int in;

        int *pid;
        int *exit_status;
    } downstream;

    /* Proxy details */
    struct {
        int id;
        int pgid;
        char *interface_env_name;
        char *hostname;

        int proxy_core_count;
        int proxy_process_count;
    } local;

    /* Process segmentation information for this proxy */
    int start_pid;
    struct HYD_proxy_exec *exec_list;
};

extern struct HYD_pmcd_pmip HYD_pmcd_pmip;

/* utils */
HYD_status HYD_pmcd_pmi_proxy_get_params(char **t_argv);
HYD_status HYD_pmcd_pmi_proxy_cleanup_params(void);
HYD_status HYD_pmcd_pmi_proxy_procinfo(int fd);
HYD_status HYD_pmcd_pmi_proxy_launch_procs(void);
void HYD_pmcd_pmi_proxy_killjob(void);

/* callback */
HYD_status HYD_pmcd_pmi_proxy_control_connect_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_proxy_control_cmd_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_proxy_stdout_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_proxy_stderr_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmcd_pmi_proxy_stdin_cb(int fd, HYD_event_t events, void *userp);

#endif /* PMI_PROXY_H_INCLUDED */
