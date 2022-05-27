#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <optee_msg_supplicant.h>
#include <tee_tpm.h>
#include <unistd.h>
#include <inttypes.h>

static int tpmtool_transmit(const uint8_t *buf, ssize_t length, uint8_t *response, ssize_t *resp_length)
{
	// ---------- Transmit command given in buf to device with handle given in dev_tpm ----------
	int ret_val = EXIT_SUCCESS;	// Return value.
	int dev_tpm = -1;		// TPM device handle.
	ssize_t transmit_size = 0;	// Amount of bytes sent to / received from the TPM.

    memset(response, 0, *resp_length);
    
    // ---------- Open TPM device ----------
	dev_tpm = open("/dev/tpm0", O_RDWR);
	/*
    if (-1 == dev_tpm)
	{
		ret_val = errno;
		fprintf(stderr, "Error opening the device.\n");
		break;
	}
    */

    // Send request data to TPM.
	transmit_size = write(dev_tpm, buf, length);
	/*
    if (transmit_size == ERR_COMMUNICATION || length != transmit_size)
	{
		ret_val = errno;
		fprintf(stderr, "Error sending request to TPM.\n");
		break;
	}
    */

    // Read the TPM response header.
	transmit_size = read(dev_tpm, response, TPM_RESP_MAX_SIZE);
	/*if (transmit_size == ERR_COMMUNICATION)
	{
		ret_val = errno;
		fprintf(stderr, "Error reading response from TPM.\n");
		break;
	}
    */
    
    // Update response buffer length with value of data length returned by TPM.
	*resp_length = transmit_size;

	// ---------- Close TPM device ----------
	if (-1 != dev_tpm)
	{
		// Close file handle.
		close(dev_tpm);

		// Invalidate file handle.
		dev_tpm = -1;
	}

	return ret_val;
}

static int buf_to_uint64(uint8_t *input_buffer, uint32_t offset, uint32_t length, uint64_t *output_value)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint32_t i = 0;			// Loop variable.
	uint64_t tmp = 0;		// Temporary variable for value calculation.

    *output_value = 0;

    for (i = 0; i < length; i++)
	{
		tmp = (tmp << 8) + input_buffer[offset + i];
	}
	*output_value = tmp;

	return ret_val;
}

static int print_capability_flags(uint8_t *response_buf, uint8_t cap_selector, int *n, char **outbuf)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint64_t propertyValue = 0;	// Value of the read property.
	uint64_t propertyKey = 0;	// Key of the property.
	int tmp = 0;			// Temporary buffer.

	do
	{
		if(cap_selector == PT_FIXED_SELECTOR)
		{
			(*n) += sprintf((*outbuf) + (*n), "\nTPM capability information of fixed properties:\n");
			(*n) += sprintf((*outbuf) + (*n), "=========================================================\n");

			for(int x = 0x13; x<(TPM_RESP_MAX_SIZE-8); x+=8)
			{	//Iterate over each property key/value pair
				ret_val = buf_to_uint64(response_buf, x, 4, &propertyKey);
				ret_val = buf_to_uint64(response_buf, x+4, 4, &propertyValue);

				switch(propertyKey)
				{
					case 0x100:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_FAMILY_INDICATOR:        %c%c%c%c\n", response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+1:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_LEVEL:                   %" PRIu64 "\n", propertyValue);
						break;
					case 0x100+2:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_REVISION:                %" PRIu64 "\n", propertyValue);
						break;
					case 0x100+3:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_DAY_OF_YEAR:             %" PRIu64 "\n", propertyValue);
						break;
					case 0x100+4:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_YEAR:                    %" PRIu64 "\n", propertyValue);
						break;
					case 0x100+5:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_MANUFACTURER:            %c%c%c%c\n", response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+6:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_VENDOR_STRING:           ");
						(*n) += sprintf((*outbuf) + (*n), "%c%c%c%c",  response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+7: // it is assumed that TPM_PT_VENDOR_STRING_2 follows _1
						(*n) += sprintf((*outbuf) + (*n), "%c%c%c%c",  response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+8:
						(*n) += sprintf((*outbuf) + (*n), "%c%c%c%c",  response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+9:
						(*n) += sprintf((*outbuf) + (*n), "%c%c%c%c\n",  response_buf[x+4], response_buf[x+5], response_buf[x+6], response_buf[x+7]);
						break;
					case 0x100+10:
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_VENDOR_TPM_TYPE:         %" PRIu64 "\n", propertyValue);
						break;
					case 0x100+11:
						// special handling for firmware version XX.xx.xxxx.x
						ret_val = buf_to_uint64(response_buf, x+4, 2, &propertyValue);
						(*n) += sprintf((*outbuf) + (*n), "TPM_PT_FIRMWARE_VERSION:        %" PRIu64 "", propertyValue);

						ret_val = buf_to_uint64(response_buf, x+6, 2, &propertyValue);
						(*n) += sprintf((*outbuf) + (*n), ".%" PRIu64 "", propertyValue);
						break;
					case 0x100+12:
						// special handling for firmware version XX.xx.xxxx.x
						ret_val = buf_to_uint64(response_buf, x+4, 2, &propertyValue); // Check for output version.

						if (2 <= propertyValue) // Infineon custom format
						{
							ret_val = buf_to_uint64(response_buf, x+5, 2, &propertyValue);
							(*n) += sprintf((*outbuf) + (*n), ".%" PRIu64 "", propertyValue);

							ret_val = buf_to_uint64(response_buf, x+7, 1, &propertyValue);
							(*n) += sprintf((*outbuf) + (*n), ".%" PRIu64 "\n", propertyValue);
						}
						else
						{
							ret_val = buf_to_uint64(response_buf, x+4, 4, &propertyValue);
							(*n) += sprintf((*outbuf) + (*n), ".%" PRIu64 "\n", propertyValue);
						}
						break;

					case 0x100+24:
						(*n) += sprintf((*outbuf) + (*n), "\nTPM_PT_MEMORY:\n");
						(*n) += sprintf((*outbuf) + (*n), "=========================================================\n");
						tmp = ((propertyValue & (1<<0)) == 0? 0:1); // Check bit 0 value.
						(*n) += sprintf((*outbuf) + (*n), "Shared RAM:                     %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<1)) == 0? 0:1); // Check bit 1 value.
						(*n) += sprintf((*outbuf) + (*n), "Shared NV:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<2)) == 0? 0:1); // Check bit 2 value.
						(*n) += sprintf((*outbuf) + (*n), "Object Copied To Ram:           %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
						//bit 31:3 = reserved
						break;

					case 0x200:
						(*n) += sprintf((*outbuf) + (*n), "\nTPM_PT_PERMANENT:\n");
						(*n) += sprintf((*outbuf) + (*n), "=========================================================\n");

						tmp = ((propertyValue & (1<<0)) == 0? 0:1); // Check bit 0 value.
						(*n) += sprintf((*outbuf) + (*n), "Owner Auth Set:                 %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<1)) == 0? 0:1); // Check bit 1 value.
						(*n) += sprintf((*outbuf) + (*n), "Sendorsement Auth Set:          %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<2)) == 0? 0:1); // Check bit 2 value.
						(*n) += sprintf((*outbuf) + (*n), "Lockout Auth Set:               %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						//bit 7:3 = reserved

						tmp = ((propertyValue & (1<<8)) == 0? 0:1); // Check bit 8 value.
						(*n) += sprintf((*outbuf) + (*n), "Disable Clear:                  %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<9)) == 0? 0:1); // Check bit 9 value.
						(*n) += sprintf((*outbuf) + (*n), "In Lockout:                     %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<10)) == 0? 0:1); // Check bit 10 value.
						(*n) += sprintf((*outbuf) + (*n), "TPM Generated EPS:              %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
						//bit 31:11 = reserved
						break;
					default:
						// Unknown attribute - ignore
						break;
				}
			}
		}
		else if (cap_selector == PT_VAR_SELECTOR)
		{
			(*n) += sprintf((*outbuf) + (*n), "\nTPM capability information of variable properties:\n");
			for(int x = 0x13; x<TPM_RESP_MAX_SIZE-8; x+=8)
			{	//Iterate over each property key/value pair
				ret_val = buf_to_uint64(response_buf, x, 4, &propertyKey);
				ret_val = buf_to_uint64(response_buf, x+4, 4, &propertyValue);

				switch(propertyKey)
				{
					case 0x201:
						(*n) += sprintf((*outbuf) + (*n), "\nTPM_PT_STARTUP_CLEAR:\n");
						(*n) += sprintf((*outbuf) + (*n), "=========================================================\n");

						tmp = ((propertyValue & (1<<0)) == 0? 0:1); // Check bit 0 value.
						(*n) += sprintf((*outbuf) + (*n), "Ph Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<1)) == 0? 0:1); // Check bit 1 value.
						(*n) += sprintf((*outbuf) + (*n), "Sh Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));

						tmp = ((propertyValue & (1<<2)) == 0? 0:1); // Check bit 2 value.
						(*n) += sprintf((*outbuf) + (*n), "Eh Enable:                      %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
						//bit 30:3 = reserved
						tmp = ((propertyValue & (1<<31)) == 0? 0:1); // Check bit 31 value.
						(*n) += sprintf((*outbuf) + (*n), "Orderly:                        %i %s", (tmp), ((tmp)? "SET\n" : "CLEAR\n"));
						break;
					default:
						// Unknown attribute - ignore
						break;
				}
			}
		}
	} while (0);

	return ret_val;
}

static int response_print(uint8_t *response_buf, int cmd, int *n, char **outbuf)
{
	int ret_val = EXIT_SUCCESS;	// Return value.

    switch (cmd) {
	case OPTEE_TPM_VERSION: // Print the fixed capability flags.
		ret_val = print_capability_flags(response_buf, PT_FIXED_SELECTOR, n, outbuf);
		break;
	default:
		break;
	}

	return ret_val;
}

int invoke_tpm(int cmd, int *size, char *outbuf)
{
    int ret_val = EXIT_SUCCESS;		// Return value.
    uint8_t *tpm_response_buf = NULL;	// Buffer for TPM response.
    ssize_t tpm_response_buf_size = 0;	// Size of tpm_response_buf.
    int n = 0;  // control sprintf to move position of outbuf

    // ---------- Allocate memory for buffer containing TPM response ----------
	tpm_response_buf_size = TPM_RESP_MAX_SIZE;
	tpm_response_buf = malloc(tpm_response_buf_size);
	// MALLOC_ERROR_CHECK(tpm_response_buf);
	memset(tpm_response_buf, 0xFF, tpm_response_buf_size);

    switch (cmd) {
    case OPTEE_TPM_VERSION:
        ret_val = tpmtool_transmit(tpm2_getcapability_fixed, sizeof(tpm2_getcapability_fixed), tpm_response_buf, &tpm_response_buf_size);
    default:
		break;
	}

    // Check for transmission errors.

    // Transmission has been successful, now get TPM return code from TPM response.
	
    // Print TPM response
	ret_val = response_print(tpm_response_buf, cmd, &n, &outbuf);

    // ---------- Cleanup ----------
	MEMSET_FREE(tpm_response_buf, tpm_response_buf_size);

    n += sprintf(outbuf + n, "\n");
	*size = n;
	*size += 1;
	return ret_val;
}
