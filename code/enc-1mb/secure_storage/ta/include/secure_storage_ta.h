/*
 * Copyright (c) 2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __SECURE_STORAGE_H__
#define __SECURE_STORAGE_H__

/* UUID of the trusted application */
#define TA_SECURE_STORAGE_UUID \
        { 0xf4e750bb, 0x1437, 0x4fbf, \
            { 0x87, 0x85, 0x8d, 0x35, 0x80, 0xc3, 0x49, 0x94 } }

/*
 * TA_SECURE_STORAGE_CMD_ENCRYPT_CHUNK - Encrypt a chunk of data
 * param[0] (memref input) Plaintext chunk data
 * param[1] (memref output) Encrypted chunk data (same size as input)
 * param[2] (value input) is_first flag (1 for first chunk, 0 for subsequent)
 * param[3] (value output) Encryption time for this chunk in microseconds
 */
#define TA_SECURE_STORAGE_CMD_ENCRYPT_CHUNK    0

/*
 * TA_SECURE_STORAGE_CMD_DECRYPT_CHUNK - Decrypt a chunk of data
 * param[0] (memref input) Encrypted chunk data
 * param[1] (memref output) Decrypted chunk data (same size as input)
 * param[2] (value input) is_first flag (1 for first chunk, 0 for subsequent)
 * param[3] (value output) Decryption time for this chunk in microseconds
 */
#define TA_SECURE_STORAGE_CMD_DECRYPT_CHUNK    1

/*
 * TA_SECURE_STORAGE_CMD_FINALIZE - Get final timing statistics
 * param[0] (value output) Total encryption time in milliseconds
 * param[1] (value output) Total decryption time in milliseconds
 * param[2] (value output) Total bytes processed
 * param[3] unused
 */
#define TA_SECURE_STORAGE_CMD_FINALIZE         2

/*
 * TA_SECURE_STORAGE_CMD_RESET - Reset session state
 * param[0-3] unused
 */
#define TA_SECURE_STORAGE_CMD_RESET            3

#endif /* __SECURE_STORAGE_H__ */