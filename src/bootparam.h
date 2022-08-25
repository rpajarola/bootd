/*
 * $Id: bootparam.h,v 1.4 2002/07/22 13:05:18 pajarola Exp $
 */

#ifndef _BOOTPARAM_H
#define _BOOTPARAM_H

/*
 * SUN rpc.bootparam Protocol
 */

void            bootparamprog_1(struct svc_req * rqstp, SVCXPRT * transp);
void            bootparam_service_init();

#endif
