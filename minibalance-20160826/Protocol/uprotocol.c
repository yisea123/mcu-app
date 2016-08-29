#include "uprotocol.h"

void poll_uart3_fifo(Ringfifo *mfifo) 
{
		char ch;
		static char pre = 0; 
		if (rfifo_len(mfifo) > 0) {
				if (rfifo_get(mfifo, &ch, 1) == 1) {
#ifdef MINIBALANCE
						if (miniTimer && pre != ch) {
							deliver_message(make_message(CMD_REMOTE_CONTROL, &ch, 1), miniTimer);
						}
#endif
						pre = ch;
				}
		}
}


