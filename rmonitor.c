/*
 * KLD Skeleton
 * Inspired by Andrew Reiter's Daemonnews article
 */

#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/kernel.h> /* types used in module initialization */
#include <sys/vmmeter.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/socket.h>

/* debug */
#define DEBUG 1
#define DEBUG_TASK (1 << 1)
#define DEBUG_VMFP (1 << 2)
#define DEBUG_PROC (1 << 3)
#define DEBUG_SOCK (1 << 4)
static unsigned int debug_level = DEBUG_SOCK;

/* sysctl */
static struct sysctl_ctx_list clist;
static struct sysctl_oid *poid;

static unsigned int period = 3;

/* vm free percentage */
static unsigned int vmfp_enable = 1;
static int vmfp = 0;
static int vmfp_low = 10;
static int vmfp_gap = 10;
static char vmfp_str[5] = "0%";

/* number of process */
static unsigned int proc_enable = 1;
static int proc = 0;
static int proc_low = 10;
static int proc_gap = 10;
static char proc_str[5] = "0%";

/* number of socket */
static unsigned int sock_enable = 1;
static int sock = 0;
static int sock_low = 10;
static int sock_gap = 10;
static char sock_str[5] = "0%";

/* callout */
static struct callout rmon_callout;

static void
rmonitor_task_detect(void *arg) {
    int pre, gap;

    if (debug_level &  DEBUG_TASK)
        printf("Rmonitor execute, period: %d secs\n", period);

    if (vmfp_enable && cnt.v_page_count) {
        pre = vmfp;
        vmfp = (cnt.v_free_count * 100) / cnt.v_page_count;
        gap = pre - vmfp;
        snprintf(vmfp_str, sizeof(vmfp_str), "%d%%", vmfp);
        if (vmfp < vmfp_low)
            printf("Rmonitor: free memory %d%% is lower than threshold %d%%\n", vmfp, vmfp_low);
        if (gap > vmfp_gap)
            printf("Rmonitor: free memory decrease %d%%(%d%%->%d%%) is greater than threshold %d%%\n", 
                    gap, pre, vmfp, vmfp_gap);
            
        if (debug_level & DEBUG_VMFP)
            printf("free vm %d total %d percent %d%%\n", cnt.v_free_count, cnt.v_page_count, vmfp);
    }

    if (proc_enable && maxproc) {
        pre = proc;
        proc = 100 - (nprocs * 100) / maxproc;
        gap = pre - proc;
        snprintf(proc_str, sizeof(proc_str), "%d%%", proc);
        if (proc < proc_low)
            printf("Rmonitor: free process %d%% is lower than threshold %d%%\n", proc, proc_low);
        if (gap > proc_gap)
            printf("Rmonitor: free process decrease %d%%(%d%%->%d%%) is greater than threshold %d%%\n",
                    gap, pre, proc, proc_gap);

        if (debug_level & DEBUG_PROC)
            printf("free proc %d total %d percent %d%%\n", (maxproc - nprocs), maxproc, proc);
    }

    if (sock_enable && maxsockets) {
        pre = sock;
        sock = 100 - (numopensockets * 100) / maxsockets;
        gap = pre - sock;
        snprintf(sock_str, sizeof(sock_str), "%d%%", sock);
        if (sock < sock_low)
            printf("Rmonitor: free socket %d%% is lower than threshold %d%%\n", sock, sock_low);
        if (gap > sock_gap)
            printf("Rmonitor: free socket decrease %d%%(%d%%->%d%%) is greater than threshold %d%%\n",
                    gap, pre, sock, sock_gap);

        if (debug_level & DEBUG_SOCK)
            printf("free sock %d total %d percent %d%%\n", (maxsockets - numopensockets), maxsockets, sock);
    }

    if (period)
        callout_reset(&rmon_callout, period*hz, rmonitor_task_detect, NULL);
}

static int
sysctl_period_procedure(SYSCTL_HANDLER_ARGS) {
    int error = 0;
    unsigned int p = period;

    error = sysctl_handle_int(oidp, &period, 0, req);
    if (error) {
        uprintf("Rmonitor: sysctl_handle_int failed\n");
        goto period_ret;
    }

    if (period != p) {
        uprintf("Rmonitor set, period: %d secs\n", period);
        if (period)
            callout_reset(&rmon_callout, period*hz, rmonitor_task_detect, NULL);
        else
            callout_drain(&rmon_callout);
    }
        
period_ret:
    return error;
}

static int
rmonitor_modevent(module_t mod __unused, int event, void *arg __unused)
{
    int err = 0;

    switch (event) {
    case MOD_LOAD:                /* kldload */
        sysctl_ctx_init(&clist);

        poid = SYSCTL_ADD_NODE(&clist,
                SYSCTL_STATIC_CHILDREN(/* tree top */), OID_AUTO,
                "rmon", 0, 0, "rmonitor root");
        if (poid == NULL) {
            uprintf("SYSCTL_ADD_NODE failed.\n");
            return (EINVAL);
        }

        SYSCTL_ADD_PROC(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "period", CTLTYPE_UINT | CTLFLAG_WR, 0, 0, sysctl_period_procedure,
            "I", "monitor period (secs), 0: disable");
            
        SYSCTL_ADD_UINT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "debug_level", CTLFLAG_RW, &debug_level, 0,  "rmonitor debug level");
            
        /* vmfp */

        SYSCTL_ADD_UINT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_enable", CTLFLAG_RW, &vmfp_enable, 0,  "enable vm free detection");

        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_low", CTLFLAG_RW, &vmfp_low, 0, "vm free percentage low threshold");
            
        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_gap", CTLFLAG_RW, &vmfp_gap, 0, "vm free percentage decrease threshold");
            
        SYSCTL_ADD_STRING(&clist, SYSCTL_CHILDREN(poid), OID_AUTO, 
                "vmfp", CTLFLAG_RD, 
                vmfp_str, sizeof(vmfp_str), "vm free percentage");

        /* proc */

        SYSCTL_ADD_UINT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "proc_enable", CTLFLAG_RW, &proc_enable, 0,  "enable proc free detection");

        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "proc_low", CTLFLAG_RW, &proc_low, 0, "proc free percentage low threshold");
            
        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "proc_gap", CTLFLAG_RW, &proc_gap, 0, "proc free percentage decrease threshold");
            
        SYSCTL_ADD_STRING(&clist, SYSCTL_CHILDREN(poid), OID_AUTO, 
                "proc", CTLFLAG_RD, 
                proc_str, sizeof(proc_str), "proc free percentage");

        /* sock */

        SYSCTL_ADD_UINT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "sock_enable", CTLFLAG_RW, &sock_enable, 0,  "enable sock free detection");

        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "sock_low", CTLFLAG_RW, &sock_low, 0, "sock free percentage low threshold");
            
        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "sock_gap", CTLFLAG_RW, &sock_gap, 0, "sock free percentage decrease threshold");
            
        SYSCTL_ADD_STRING(&clist, SYSCTL_CHILDREN(poid), OID_AUTO, 
                "sock", CTLFLAG_RD, 
                sock_str, sizeof(sock_str), "sock free percentage");

        /* callout */

        callout_init(&rmon_callout, CALLOUT_MPSAFE);
        callout_reset(&rmon_callout, 0, rmonitor_task_detect, NULL);

        uprintf("Rmonitor KLD loaded, use 'sysctl rmon' to execute.\n");
        break;
    case MOD_UNLOAD:
        callout_stop(&rmon_callout);
        if (sysctl_ctx_free(&clist)) {
            uprintf("sysctl_ctx_free failed.\n");
            return (ENOTEMPTY);
        }

        uprintf("Rmonitor KLD unloaded.\n");
        break;
    default:
        err = EOPNOTSUPP;
        break;
    }
  return(err);
}

/* Declare this module to the rest of the kernel */

static moduledata_t rmonitor_mod = {
    "rmonitor",
    rmonitor_modevent,
    NULL
};

DECLARE_MODULE(rmonitor, rmonitor_mod, SI_SUB_KLD, SI_ORDER_ANY);
