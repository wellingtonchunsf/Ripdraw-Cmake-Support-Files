#include <stdio.h>
#include "../extint/ripdraw.h"

int load_1_root_file_name(RD_INTERFACE* rd_interface);

int main(int argc, char **argv)
{
	int ret;
       RD_INTERFACE* rd_interface = RdInterfaceInit("COM6");
	if (argc > 1)
	{
		rd_interface->verbose = atoi(argv[1]);
		if (rd_interface->verbose < 0)
		{
			rd_interface->verbose = 0;
		}
	}

	ret = Rd_Reset(rd_interface);
	if (ret >= 0)
	{
		ret = load_1_root_file_name(rd_interface);

		ret = Rd_ComposeLayersToPage(rd_interface, 1);
		if (ret >= 0)
		{
			ret = Rd_PageToScreen(rd_interface, 1);
		}
	}
	RdInterfaceClose(rd_interface);

	printf("\nRet: %d\n", ret);
	printf("Done!\n");
	return 0;
}
