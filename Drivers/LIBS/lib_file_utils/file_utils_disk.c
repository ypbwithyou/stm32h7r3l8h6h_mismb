#include "file_utils.h"

int8_t IdaGetDiskSpaceKB(float *total_kb, float *free_kb)
{
    FATFS *fs_ptr = NULL;
    DWORD free_clusters = 0U;
    DWORD total_sectors;
    DWORD free_sectors;
    uint32_t sector_size;
    FRESULT res;

    if ((total_kb == NULL) || (free_kb == NULL))
    {
        return -1;
    }

    res = f_getfree("0:", &free_clusters, &fs_ptr);
    if ((res != FR_OK) || (fs_ptr == NULL) || (fs_ptr->n_fatent <= 2U))
    {
        *total_kb = 0.0f;
        *free_kb = 0.0f;
        return -1;
    }

#if FF_MAX_SS != FF_MIN_SS
    sector_size = (uint32_t)fs_ptr->ssize;
#else
    sector_size = (uint32_t)FF_MAX_SS;
#endif

    total_sectors = (DWORD)(fs_ptr->n_fatent - 2U) * (DWORD)fs_ptr->csize;
    free_sectors = free_clusters * (DWORD)fs_ptr->csize;

    *total_kb = ((float)((uint64_t)total_sectors * (uint64_t)sector_size)) / 1024.0f;
    *free_kb = ((float)((uint64_t)free_sectors * (uint64_t)sector_size)) / 1024.0f;
    return 0;
}
