#include <plugin/kPlugin.h>

static
void kGuest(void)
{
    kPluginPrintf("[Workhorse guest]: hello world!\n");    
}

K_REGISTER_PLUGIN(demo, kGuest, 000);