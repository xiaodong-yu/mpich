/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

struct HYDT_bsci_fns HYDT_bsci_fns;
struct HYDT_bsci_info HYDT_bsci_info;

/* *INDENT-OFF* */
static const char *launcher_array[] = {  "ssh",  "rsh",  "fork",  "slurm",  "ll",  "lsf",  "sge",  "manual",  "persist",  NULL };
static const char *rmk_array[] = {  "user",  "slurm",  "ll",  "lsf",  "sge",  "pbs",  "cobalt",  NULL };
static HYD_status(*launcher_init_array[])(void) = {  HYDT_bsci_launcher_ssh_init,  HYDT_bsci_launcher_rsh_init,  HYDT_bsci_launcher_fork_init,  HYDT_bsci_launcher_slurm_init,  HYDT_bsci_launcher_ll_init,  HYDT_bsci_launcher_lsf_init,  HYDT_bsci_launcher_sge_init,  HYDT_bsci_launcher_manual_init,  HYDT_bsci_launcher_persist_init,  NULL };
static HYD_status(*rmk_init_array[])(void) = {  HYDT_bsci_rmk_user_init,  HYDT_bsci_rmk_slurm_init,  HYDT_bsci_rmk_ll_init,  HYDT_bsci_rmk_lsf_init,  HYDT_bsci_rmk_sge_init,  HYDT_bsci_rmk_pbs_init,  HYDT_bsci_rmk_cobalt_init,  NULL };
/* *INDENT-ON* */

static void init_rmk_fns(void)
{
    HYDT_bsci_fns.query_native_int = NULL;
    HYDT_bsci_fns.query_node_list = NULL;
    HYDT_bsci_fns.rmk_finalize = NULL;
}

static void init_launcher_fns(void)
{
    HYDT_bsci_fns.launch_procs = NULL;
    HYDT_bsci_fns.launcher_finalize = NULL;
    HYDT_bsci_fns.wait_for_completion = NULL;
    HYDT_bsci_fns.query_proxy_id = NULL;
    HYDT_bsci_fns.query_env_inherit = NULL;
}

static void init_bsci_info(void)
{
    HYDT_bsci_info.rmk = NULL;
    HYDT_bsci_info.launcher = NULL;
    HYDT_bsci_info.launcher_exec = NULL;
    HYDT_bsci_info.enablex = -1;
    HYDT_bsci_info.debug = -1;
}

static const char *detect_rmk(int debug)
{
    int i, ret = 0;
    HYD_status status = HYD_SUCCESS;

    for (i = 0; rmk_array[i]; i++) {
        init_rmk_fns();

        HYDT_bsci_info.rmk = rmk_array[i];
        HYDT_bsci_info.debug = debug;

        status = (*rmk_init_array[i]) ();
        HYDU_ERR_POP(status, "bootstrap device returned error initializing\n");

        status = HYDT_bsci_query_native_int(&ret);
        HYDU_ERR_POP(status, "unable to query native environment\n");

        if (HYDT_bsci_fns.rmk_finalize) {
            status = HYDT_bsci_fns.rmk_finalize();
            HYDU_ERR_POP(status, "unable to finalize bootstrap server\n");
        }

        init_rmk_fns();

        if (ret)
            break;
    }

  fn_exit:
    if (ret)
        return ((const char *) rmk_array[i]);
    else
        return NULL;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bsci_init(const char *user_rmk, const char *user_launcher,
                          const char *user_launcher_exec, int enablex, int debug)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize the RMK and launcher functions */
    init_bsci_info();
    init_rmk_fns();
    init_launcher_fns();


    /* See if the user specified an RMK */
    if (user_rmk)
        HYDT_bsci_info.rmk = user_rmk;
    if (HYDT_bsci_info.rmk == NULL)
        MPL_env2str("HYDRA_RMK", (const char **) &HYDT_bsci_info.rmk);

    /* User didn't specify an RMK; try to detect one */
    if (HYDT_bsci_info.rmk == NULL)
        HYDT_bsci_info.rmk = detect_rmk(debug);


    /* See if the user specified a launcher */
    if (user_launcher)
        HYDT_bsci_info.launcher = user_launcher;
    if (HYDT_bsci_info.launcher == NULL)
        MPL_env2str("HYDRA_LAUNCHER", (const char **) &HYDT_bsci_info.launcher);
    if (HYDT_bsci_info.launcher == NULL)
        MPL_env2str("HYDRA_BOOTSTRAP", (const char **) &HYDT_bsci_info.launcher);

    /* User didn't specify a launcher; try to see if the RMK also
     * provides launcher capabilities */
    if (HYDT_bsci_info.rmk && HYDT_bsci_info.launcher == NULL) {
        for (i = 0; launcher_array[i]; i++)
            if (!strcmp(HYDT_bsci_info.rmk, launcher_array[i]))
                break;
        if (launcher_array[i])
            HYDT_bsci_info.launcher = launcher_array[i];
    }

    /* If no launcher is provided or detected, use the default launcher */
    if (HYDT_bsci_info.launcher == NULL)
        HYDT_bsci_info.launcher = HYDRA_DEFAULT_LAUNCHER;

    if (HYDT_bsci_info.launcher == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no appropriate launcher found\n");

    HYDT_bsci_info.debug = debug;
    HYDT_bsci_info.enablex = enablex;

    if (user_launcher_exec)
        HYDT_bsci_info.launcher_exec = user_launcher_exec;
    if (HYDT_bsci_info.launcher_exec == NULL) {
        if (MPL_env2str("HYDRA_LAUNCHER_EXEC", &HYDT_bsci_info.launcher_exec) == 0)
            HYDT_bsci_info.launcher_exec = NULL;
    }
    if (HYDT_bsci_info.launcher_exec == NULL) {
        if (MPL_env2str("HYDRA_BOOTSTRAP_EXEC", &HYDT_bsci_info.launcher_exec) == 0)
            HYDT_bsci_info.launcher_exec = NULL;
    }

    /* Make sure the launcher we found is valid */
    for (i = 0; launcher_array[i]; i++) {
        if (!strcmp(HYDT_bsci_info.launcher, launcher_array[i])) {
            status = (*launcher_init_array[i]) ();
            HYDU_ERR_POP(status, "launcher init returned error\n");
            break;
        }
    }
    if (launcher_array[i] == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unrecognized launcher: %s\n", HYDT_bsci_info.launcher);


    /* If no RMK is provided or detected, use the default RMK */
    if (HYDT_bsci_info.rmk == NULL)
        HYDT_bsci_info.rmk = HYDRA_DEFAULT_RMK;

    /* Initialize the RMK */
    for (i = 0; rmk_array[i]; i++) {
        if (!strcmp(HYDT_bsci_info.rmk, rmk_array[i])) {
            status = (*rmk_init_array[i]) ();
            HYDU_ERR_POP(status, "RMK init returned error\n");
            break;
        }
    }
    if (rmk_array[i] == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "unrecognized RMK: %s\n", HYDT_bsci_info.rmk);


    /* This function is mandatory */
    if (HYDT_bsci_fns.launch_procs == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "mandatory launch function undefined\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
