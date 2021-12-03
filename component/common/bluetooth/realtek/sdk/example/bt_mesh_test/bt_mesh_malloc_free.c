#include "platform_os.h"
#include "bt_mesh_user_api.h"
#include "bt_mesh_malloc_free.h"

void *bt_mesh_test_malloc(size_t size)
{
	void *ptr = (void *)os_mem_zalloc(RAM_TYPE_DATA_ON, size);
	return ptr;
}

void bt_mesh_test_free(void *ptr)
{
	os_mem_free((u8 *)ptr);
}
