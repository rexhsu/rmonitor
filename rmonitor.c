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

/* debug */
#define DEBUG 1
#define DEBUG_TASK (1 << 1)
#define DEBUG_VMFP (1 << 2)
static unsigned int debug_level = 1;

/* vm */
extern struct vmmeter cnt;

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
static int proc_high = 10;
static int proc_gap = 10;

/* callout */
static struct callout rmon_callout;

static void
rmonitor_task_detect(void *arg) {
    int pre_vmfp, gap;

    if (debug_level &  DEBUG_TASK)
        printf("Rmonitor execute, period: %d secs\n", period);

    if (vmfp_enable && cnt.v_page_count) {
        pre_vmfp = vmfp;
        vmfp = (cnt.v_free_count*100) / cnt.v_page_count;
        gap = pre_vmfp - vmfp;
        snprintf(vmfp_str, sizeof(vmfp_str), "%d%%", vmfp);
        if (vmfp < vmfp_low)
            printf("Rmonitor: free memory %d%% is lower than threshold %d%%\n", vmfp, vmfp_low);
        if (gap > vmfp_gap)
            printf("Rmonitor: free memory decrease %d%%(%d%%->%d%%) is greater than threshold %d%%\n", 
                    gap, pre_vmfp, vmfp, vmfp_gap);
            
        if (debug_level & DEBUG_VMFP)
            printf("free %d total %d percent %d%%\n", cnt.v_free_count, cnt.v_page_count, vmfp);
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
            
        SYSCTL_ADD_UINT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_enable", CTLFLAG_RW, &vmfp_enable, 0,  "enable vm free detection");

        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_low", CTLFLAG_RW, &vmfp_low, 0, "vm free percentage low threshold");
            
        SYSCTL_ADD_INT(&clist, SYSCTL_CHILDREN(poid), OID_AUTO,
            "vmfp_gap", CTLFLAG_RW, &vmfp_gap, 0, "vm free percentage decrease threshold");
            
        SYSCTL_ADD_STRING(&clist, SYSCTL_CHILDREN(poid), OID_AUTO, 
                "vmfp", CTLFLAG_RD, 
                vmfp_str, sizeof(vmfp_str), "vm free percentage");

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
