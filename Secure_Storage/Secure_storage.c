#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

TEE_Result TA_CreateEntryPoint(void) { return TEE_SUCCESS; }
void TA_DestroyEntryPoint(void) {}
TEE_Result TA_OpenSessionEntryPoint(uint32_t p1, uint32_t p2, 
                                    const void *p3, void **sess_ctx) { 
    return TEE_SUCCESS; 
}
void TA_CloseSessionEntryPoint(void *sess_ctx) {}

TEE_Result TA_InvokeCommandEntryPoint(void *sess_ctx, uint32_t cmd_id, 
                                      uint32_t p_types, TEE_Param p[4]) {
    (void)sess_ctx; (void)p_types;
    char data[] = "Hello Secure Storage!";
    TEE_ObjectHandle object;
    TEE_Result res;

    switch (cmd_id) {
    case 0:
        res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                         "myfile", sizeof("myfile"),
                                         TEE_DATA_FLAG_ACCESS_READ |
                                         TEE_DATA_FLAG_ACCESS_WRITE |
                                         TEE_DATA_FLAG_OVERWRITE,
                                         TEE_HANDLE_NULL, data, sizeof(data),
                                         &object);
        if (res == TEE_SUCCESS)
            TEE_CloseObject(object);
        return res;

    case 1:
        res = TEE_OpenPersistentObject(TEE_STORAGE_PRIVATE,
                                       "myfile", sizeof("myfile"),
                                       TEE_DATA_FLAG_ACCESS_READ, &object);
        if (res == TEE_SUCCESS) {
            char buffer[64];
            uint32_t bytes;
            TEE_ReadObjectData(object, buffer, sizeof(buffer), &bytes);
            TEE_CloseObject(object);
        }
        return res;
    }
    return TEE_ERROR_BAD_PARAMETERS;
}

