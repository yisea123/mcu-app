#include <stdio.h>
#include <stdlib.h>
/* $Id $ */

void proc_head (char *mes)
{
    printf("\r\n\
**************************************************************\r\n\
\r\n\
     European digital cellular telecommunications system\r\n\
           4750 ... 12200 bits/s speech codec for\r\n\
         Adaptive Multi-Rate speech traffic channels\r\n\
\r\n\
     Bit-Exact C Simulation Code - %s\r\n\
\r\n\
     R98:   Version 7.6.0  \r\n\
     R99:   Version 3.3.0  \r\n\
     REL-4: Version 4.1.0   December 12, 2001\r\n\
     REL-5: Version 5.1.0   March 26, 2003\r\n\
**************************************************************\r\n\r\n",
            mes);

}
