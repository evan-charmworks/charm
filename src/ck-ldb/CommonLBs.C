/**
 * \addtogroup CkLdb
*/
/*@{*/

/*
 Startup routine for use when you include the commonly used load balancers.
*/
#include <LBManager.h>
#include "CommonLBs.decl.h"

static void CreateNoLB(void)
{
	/*empty-- let the user create the load balancer*/
}

void initCommonLBs(void) {
#if CMK_LBDB_ON
//  LBSetDefaultCreate(CreateNoLB);
#endif
}

#include "CommonLBs.def.h"

/*@}*/
