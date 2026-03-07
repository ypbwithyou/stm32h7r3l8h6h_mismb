#ifndef MCU_LIB_INI_FATFS_INCLUDE_INI_FATFS_H_
#define MCU_LIB_INI_FATFS_INCLUDE_INI_FATFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INI_RET_OK              (0)
#define INI_RET_ERROR           (-1)

char* StrTrim(char* str);

int LoadConfFile(const char* file);
int GetSecStr(const char* sector, const char* key, char* value);
int SetSecStr(const char* sector, const char* key, char* value);
int GetSecInt(const char* sector, const char* key, int* value);
int SetSecInt(const char* sector, const char* key, int* value);
int GetSecFloat(const char* sector, const char* key, float* value);
int SetSecFloat(const char* sector, const char* key, float* value);
int GetSecDouble(const char* sector, const char* key, double* value);
int SetSecDouble(const char* sector, const char* key, double* value);

#ifdef __cplusplus
}
#endif

#endif /* MCU_LIB_INI_FATFS_INCLUDE_INI_FATFS_H_ */
