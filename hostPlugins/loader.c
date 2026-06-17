#include <plugin/kPlugin.h>
#include <ia32eVma.h>
#include <ia32e.h>
#include <ia32eCpu.h>
#include <lib/acpi.h>

#include "workhorse.bin"

extern 
unsigned char ia32eBiosWakeupBlobStart[];

extern 
unsigned char ia32eBiosWakeupBlobEnd[];

#define IMAGE_NUM_PAGES ((sizeof(image) + 4095) / 4096)

typedef struct
{
    ia32eVtxVmcsRegion_t ATTR_ALIGNED(4096) vmcs;
    kPluginTaskThreadParam_t param;
    kSchedTask_t task;
} workhorseVcpu_t;

static 
ia32ePml4e_t ATTR_ALIGNED(4096) pml4[512];

static 
ia32ePdpte_t ATTR_ALIGNED(4096) pdpt[512];

static 
ia32ePde_t ATTR_ALIGNED(4096) pd[512];

static
ia32ePte_t ATTR_ALIGNED(4096) pt[512];

static
kDomain_t workhorseDom;

static
kPluginDomainParam_t workhorseDomP;

static 
workhorseVcpu_t vcpus[2];

static
void workhorseLoaderMain(void)
{
    uint32_t i = 0;

    kDbgStr("[WORKHORSE LOADER]: entered\n");

    /* EPTs */

    pml4[0] = ia32eVirtToPhysStatic(pdpt) | IA32E_EPT_ENTRY_R_MASK | IA32E_EPT_ENTRY_W_MASK | IA32E_EPT_ENTRY_X_MASK;
    pdpt[0] = ia32eVirtToPhysStatic(pd) | IA32E_EPT_ENTRY_R_MASK | IA32E_EPT_ENTRY_W_MASK | IA32E_EPT_ENTRY_X_MASK;
    pd[0] = ia32eVirtToPhysStatic(pt) | IA32E_EPT_ENTRY_R_MASK | IA32E_EPT_ENTRY_W_MASK | IA32E_EPT_ENTRY_X_MASK;

    STATIC_ASSERT(IMAGE_NUM_PAGES <= ARRAY_LEN(pt)); 

    K_DYNAMIC_ASSERT((ia32eVirtToPhysStatic(image) & 0xfff) == 0);

    STATIC_ASSERT(sizeof(image) > 4096);

    K_DYNAMIC_ASSERT((4096ULL + (ia32eBiosWakeupBlobEnd - ia32eBiosWakeupBlobStart)) < entry);

    for (i = 0; i < IMAGE_NUM_PAGES; i++)
        pt[i] = ia32eVirtToPhysStatic((&image[i * 4096])) | IA32E_EPT_ENTRY_R_MASK | IA32E_EPT_ENTRY_W_MASK | 
                IA32E_EPT_ENTRY_X_MASK | (IA32E_EPT_ENTRY_MEMTYPE_WB << IA32E_EPT_ENTRY_MEMTYPE_SHIFT);

    memcpy(image, &entry, sizeof(entry));
    memcpy(&image[0x1000], ia32eBiosWakeupBlobStart, ia32eBiosWakeupBlobEnd - ia32eBiosWakeupBlobStart);

    /* Dom params */

    workhorseDomP.param.invocationInfo._start = 0x1000;
    workhorseDomP.archParam.ia32eParam.pml4BasePhys = ia32eVirtToPhysStatic(pml4);
    workhorseDomP.archParam.ia32eParam.pml4BaseVirt = (void *)pml4;
    workhorseDomP.archParam.ia32eParam.vm = true;

    /* Task params */

    for (i = 0; i < ARRAY_LEN(vcpus); i++) {
#if CONFIG_KDYNAMIC_ASSERT
        K_DYNAMIC_ASSERT((ia32eVirtToPhysStatic(&vcpus[i].vmcs) & 0xfff) == 0);
#endif

        vcpus[i].param.taskId = i;
        vcpus[i].param.period = 10;
        vcpus[i].param.budget = 10;
        vcpus[i].param.param.paramRr.timesliceTicks = 10;
        vcpus[i].param.cpuId = 0;
        vcpus[i].param.archParam.ia32eParam.vtxParam.vmcsPhys = ia32eVirtToPhysStatic(&vcpus[i].vmcs);
        vcpus[i].param.archParam.ia32eParam.vtxParam.vmcsVirt = &vcpus[i].vmcs;
    }

    /* Init */

    if (kPluginInitDomain(&workhorseDom, &workhorseDomP) < 0) {
        kDbgStrf("[WORKHORSE LOADER FAILURE]: failed to initialize domain\n");
        return;
    }

    for (i = 0; i < ARRAY_LEN(vcpus); i++) {
        if (kPluginInitTaskThread(&vcpus[i].task, &vcpus[i].param) < 0) {
            kDbgStrf("[WORKHORSE LOADER FAILURE]: failed to initialize task %u\n", i);
            return;
        }
    }

    kDbgStr("[WORKHORSE LOADER SUCCESS] initialized everything successfully\n");
}

K_REGISTER_PLUGIN(workhorseLoader, workhorseLoaderMain, 000);