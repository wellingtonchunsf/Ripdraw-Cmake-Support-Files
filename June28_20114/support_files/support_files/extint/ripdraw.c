/* ripdraw.cpp
 * 
 * supports Windows/Linux only
 * supports little-endian CPU only
 * 
 */
#include "ripdraw.h"

#define RD_PROTO_POS_CMD			0
#define RD_PROTO_POS_SEQ			2
#define RD_PROTO_POS_PL				4
#define RD_PROTO_POS_BYTE_0			6
#define RD_PROTO_POS_BYTE_1			8

int rd_extint_open(RD_INTERFACE* rd_interface, const char* port_name);
int rd_extint_close(RD_INTERFACE* rd_interface);
int rd_extint_write(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len);
int rd_extint_read(RD_INTERFACE* rd_interface, RD_BYTE* data_ptr, int data_len);

/* ================================================================== */
/* calculate the checksum */
RD_UWORD rd_checksum(RD_BYTE* data, int length)
{
    int i;
    RD_UWORD ret = 0;
    for (i = 0; i < length; i++)
    {
        ret += data[i];
    }
    return ret;
}

/* ================================================================== */
/* append byte parameter to request */
int rd_buffer_check_and_allocate(RD_INTERFACE_BUFFER* buffer, int required_capacity)
{
    RD_BYTE* tmp;
    if (buffer == NULL)
    {
        fprintf(stderr, "buffer should not NULL\n");
        return -010201;
    }
    /* required space available in buffer, if not then allocate it */
    if ((buffer->ptr) && (buffer->capacity >= required_capacity))
    {
        return 0;
    }
    required_capacity = (required_capacity < 32) ? 32 : required_capacity;
    tmp = (RD_BYTE*) malloc(required_capacity);
    if (!tmp)
    {
        fprintf(stderr, "Insufficient resource\n");
        return -010202;
    }
    /* copy old ptr to new ptr */
    if ((buffer->ptr) && (buffer->capacity > 0))
    {
        memcpy(tmp, buffer->ptr, buffer->capacity);
        free(buffer->ptr);
    }
    buffer->ptr = tmp;
    buffer->capacity = required_capacity;
    return 0;
}

/* ================================================================== */
/* append byte parameter to request */
int rd_cmd_request_append_byte(RD_INTERFACE* rd_interface, RD_BYTE input)
{
    int ret;
    _RD_CHECK_INTERFACE();
    ret = rd_buffer_check_and_allocate(&rd_interface->request, rd_interface->request.size + 1);
    if (ret < 0)
    {
        return ret;
    }
    *(rd_interface->request.ptr + rd_interface->request.size) = input;
    rd_interface->request.size++;
    return 0;
}

/* ================================================================== */
/* append uword parameter to request */
int rd_cmd_request_append_uword(RD_INTERFACE* rd_interface, RD_UWORD input)
{
    int ret;
    _RD_CHECK_INTERFACE();
    ret = rd_buffer_check_and_allocate(&rd_interface->request, rd_interface->request.size + 2);
    if (ret < 0)
    {
        return ret;
    }
    *((RD_UWORD*)(rd_interface->request.ptr + rd_interface->request.size)) = input;
    rd_interface->request.size += 2;
    return 0;
}

/* ================================================================== */
/* append flag parameter to request */
int rd_cmd_request_append_flag(RD_INTERFACE* rd_interface, RD_FLAG input)
{
    RD_BYTE tmp = (input != RD_FALSE) ? 1 : 0;
    return rd_cmd_request_append_byte(rd_interface, tmp);
}

/* ================================================================== */
/* append color parameter to request */
int rd_cmd_request_append_color(RD_INTERFACE* rd_interface, RD_COLOR input)
{
    int ret;
    ret = rd_cmd_request_append_byte(rd_interface, input.red);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, input.green);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, input.blue);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_request_append_byte(rd_interface, input.alpha);
}

/* ================================================================== */
/* append vertical direction parameter to request */
int rd_cmd_request_append_hdirection(RD_INTERFACE* rd_interface, RD_HDIRECTION input)
{
    RD_BYTE tmp = (input != RD_HDIRECTION_LEFT) ? 1 : 0;
    return rd_cmd_request_append_byte(rd_interface, tmp);
}

/* ================================================================== */
/* append direction parameter to request */
int rd_cmd_request_append_direction(RD_INTERFACE* rd_interface, RD_DIRECTION input)
{
    RD_BYTE tmp = (input != RD_DIRECTION_HORIZONTAL) ? 1 : 0;
    return rd_cmd_request_append_byte(rd_interface, tmp);
}

/* ================================================================== */
/* append string parameter to request */
int rd_cmd_request_append_string(RD_INTERFACE* rd_interface, const char* input)
{
    int ret;
    int len = strlen(input);
    ret = rd_cmd_request_append_uword(rd_interface, len);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_buffer_check_and_allocate(&rd_interface->request, rd_interface->request.size + len);
    if (ret < 0)
    {
        return ret;
    }
    memcpy(rd_interface->request.ptr + rd_interface->request.size, input, len);
    rd_interface->request.size += len;
    return 0;
}

/* ================================================================== */
/* get uword from response at given byte position */
int rd_cmd_response_check_and_get_uword(RD_INTERFACE* rd_interface, int byte_position, RD_UWORD* output)
{
    int check_bytes;
    _RD_CHECK_INTERFACE();

    check_bytes = byte_position + 2;
    if (rd_interface->response.ptr == NULL || (rd_interface->response.size < check_bytes))
    {
        fprintf(stderr, "invalid response received\n");
        return -011001;
    }
    *output = *((RD_UWORD*) (rd_interface->response.ptr + byte_position));
    return 0;
}

/* ================================================================== */
/* get byte from response at given byte position */
int rd_cmd_response_check_and_get_byte(RD_INTERFACE* rd_interface, int byte_position, RD_BYTE* output)
{
    int check_bytes;
    _RD_CHECK_INTERFACE();

    check_bytes = byte_position + 1;
    if (rd_interface->response.ptr == NULL || (rd_interface->response.size < check_bytes))
    {
        fprintf(stderr, "invalid response received\n");
        return -011101;
    }
    *output = *((RD_BYTE*) (rd_interface->response.ptr + byte_position));
    return 0;
}

/* ================================================================== */
/* get data from response at given byte position */
/* it is up to the user to free allocated memory */
int rd_cmd_response_check_and_get_data(RD_INTERFACE* rd_interface, int byte_position, char** output)
{
    int ret;
    int check_bytes;
    RD_UWORD length;
	char* tmp;

    _RD_CHECK_INTERFACE();

    ret = rd_cmd_response_check_and_get_uword(rd_interface, byte_position - 2, &length);
    if (ret < 0)
    {
        return ret;
    }
    check_bytes = byte_position + length;
    if (rd_interface->response.ptr == NULL || (rd_interface->response.size < check_bytes))
    {
        fprintf(stderr, "invalid response received\n");
        return -011201;
    }
    *output = NULL;
    tmp = (char*) malloc(length);
    if (!tmp)
    {
        fprintf(stderr, "unable to allocate memory\n");
        return -011202;
    }
    memcpy(tmp, rd_interface->response.ptr + byte_position, length);
    *output = tmp;
    return 0;
}

/* ================================================================== */
/* get event data from buffer */
/* it is up to the user to free allocated memory */
int rd_cmd_response_check_and_get_event_data(RD_INTERFACE* rd_interface, RD_EVENT** event, int byte_position, RD_UWORD* count)
{
    int ret;
    int check_bytes;
	RD_UWORD packet_count = 0;
	RD_EVENT* temp_event;
	int i;

    _RD_CHECK_INTERFACE();
   
    ret = rd_cmd_response_check_and_get_uword(rd_interface, byte_position, &packet_count);
    if (ret < 0)
    {
        return ret;
    }
    byte_position += 2;

    if (packet_count > 0)
    {
        RD_UWORD has_more_data = 0;
        ret = rd_cmd_response_check_and_get_uword(rd_interface, byte_position, &has_more_data);
        if (ret < 0)
        {
            return ret;
        }
        byte_position += 2;

       temp_event = (RD_EVENT*) malloc(packet_count * sizeof(RD_EVENT));
        if (!temp_event)
        {
            fprintf(stderr, "unable to allocate memory\n");
            return -011301;
        }
        memset(temp_event, 0, sizeof(RD_EVENT) * packet_count);

        for (i = 0; i < packet_count; i++)
        {
            RD_UWORD length;
            ret = rd_cmd_response_check_and_get_uword(rd_interface, byte_position, &length);
            if (ret < 0)
            {
                return ret;
            }
            byte_position += 2;
            length = length - 1;

            ret = rd_cmd_response_check_and_get_byte(rd_interface, byte_position, &temp_event[i].event_type);
            if (ret < 0)
            {
                return ret;
            }
            byte_position += 1;

            check_bytes = byte_position + length;
            if (rd_interface->response.ptr == NULL || (rd_interface->response.size < check_bytes))
            {
                fprintf(stderr, "invalid response received\n");
                return -011302;
            }

            temp_event[i].data = (RD_BYTE*) malloc(length);
            if (!temp_event[i].data)
            {
                fprintf(stderr, "unable to allocate memory\n");
                return -011303;
            }
            memcpy(temp_event[i].data, rd_interface->response.ptr + byte_position, length);
            byte_position += length;

            temp_event[i].has_more_data = has_more_data;
        }
        *event = temp_event;
    }
    *count = packet_count;
    return 0;
}

/* ================================================================== */
/* initialize new request */
int rd_cmd_request_init(RD_INTERFACE* rd_interface, RD_COMMAND_IDS cmd_id)
{
    int ret;
    _RD_CHECK_INTERFACE();

    rd_interface->request.size = 0;
    rd_interface->response.size = 0;
    /* add command id */
	rd_interface->last_cmd_id = cmd_id;
    ret = rd_cmd_request_append_uword(rd_interface, (RD_UWORD) cmd_id);
    if (ret < 0)
    {
        return ret;
    }
    /* add sequence number */  
	rd_interface->seq_no++;
    ret = rd_cmd_request_append_uword(rd_interface, rd_interface->seq_no);
	if (ret < 0)
    {
        return ret;
    }
	/* add dummy payload length, later on while sending update it */
    return rd_cmd_request_append_uword(rd_interface, 0);
}

/* ================================================================== */
/* send command to device */
int rd_cmd_request_process(RD_INTERFACE* rd_interface)
{
    int ret, i;
    RD_UWORD payload_len;
    RD_UWORD checksum;
    _RD_CHECK_INTERFACE();

    payload_len = rd_interface->request.size - RD_PROTO_POS_BYTE_0;
    /* update data length */
    *((RD_UWORD*) (rd_interface->request.ptr + RD_PROTO_POS_PL)) = payload_len;
    /* checksum */
    checksum = rd_checksum(rd_interface->request.ptr, rd_interface->request.size);
    ret = rd_cmd_request_append_uword(rd_interface, checksum);
    if (ret < 0)
    {
        return ret;
    }

	if (rd_interface->verbose >= 2)
	{
		printf("write: %d\n", rd_interface->request.size);
		for (i = 0; i < rd_interface->request.size; i++)
		{
			printf("%02X ",  rd_interface->request.ptr[i]);
		}
		printf("\n");
	}
    /* send to device */
	ret = rd_extint_write(rd_interface, rd_interface->request.ptr, rd_interface->request.size);
	RD_DBG(3, "write: %d done\n", ret);
	return ret;
}

/* ================================================================== */
/* receive command response */
int rd_cmd_response_receive(RD_INTERFACE* rd_interface)
{
    int ret, i, retry_count;
	RD_ID cmd_id;
	RD_UWORD seq_no;
    RD_UWORD payload_len;
    RD_UWORD checksum;
    RD_UWORD response_checksum;
    _RD_CHECK_INTERFACE();

    ret = rd_buffer_check_and_allocate(&rd_interface->response, 16);
    if (ret < 0)
    {
        return ret;
    }

	RD_DBG(2, "reading response\n");
	retry_count = 0;
retry:

	/* sleep to give time device for processing */
	RD_SLEEP(3);
    /* read command id and check with last id */
	if (retry_count == 0)
	{
		ret = rd_extint_read(rd_interface, rd_interface->response.ptr, 2);
	}
	else
	{
		/* remove first byte and read next byte */
		rd_interface->response.ptr[0] = rd_interface->response.ptr[1];
		ret = rd_extint_read(rd_interface, rd_interface->response.ptr + 1, 1);
	}
    if (ret < 0)
    {
        return ret;
    }
	rd_interface->response.size = 2;

	if (rd_interface->verbose >= 3)
	{
		for (i = 0; i < rd_interface->response.size; i++)
		{
			printf("0x%X ", rd_interface->response.ptr[i]);
		}
	}

	ret = rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_CMD, &cmd_id);
    if (ret < 0)
    {
        return ret;
    }
	if (rd_interface->last_cmd_id != cmd_id)
	{
		if (retry_count > 2000)
		{
			fprintf(stderr, "timeout, not response from device\n");
			return -011601;
		}
		retry_count++;
		goto retry;
	}

	 /* read sequence number and payload length */
	ret = rd_extint_read(rd_interface, rd_interface->response.ptr + 2, 4);
	if (ret < 0)
    {
        return ret;
    }
    rd_interface->response.size = RD_PROTO_POS_BYTE_0;

	if (rd_interface->verbose >= 2)
	{
		printf("read: %d\n", rd_interface->response.size);
		for (i = 0; i < rd_interface->response.size; i++)
		{
			printf("%d: 0x%X\n", i, rd_interface->response.ptr[i]);
		}
	}

	// check seq
	ret = rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_SEQ, &seq_no);
	if (ret < 0)
    {
        return ret;
    }
	if (rd_interface->seq_no != seq_no)
	{
		fprintf(stderr, "sequence number not match %d != %d\n", rd_interface->seq_no, seq_no);
		return -011602;
	}
    /* extract payload length */
    ret = rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_PL, &payload_len);
    if (ret < 0)
    {
        return ret;
    }
    /* required space available in response buffer, if not then allocate it */
    ret = rd_buffer_check_and_allocate(&rd_interface->response, payload_len + RD_PROTO_POS_BYTE_0);
    if (ret < 0)
    {
        return ret;
    }
    /* read payload and checksum */
    ret = rd_extint_read(rd_interface, rd_interface->response.ptr + RD_PROTO_POS_BYTE_0, payload_len + 2);
    if (ret < 0)
    {
        return ret;
    }
    rd_interface->response.size = payload_len + RD_PROTO_POS_BYTE_0 + 2;

	if (rd_interface->verbose >= 2)
	{
		for (i = 0; i < rd_interface->response.size; i++)
		{
			printf("%d: 0x%X\n", i, rd_interface->response.ptr[i]);
		}
	}

	/* extract checksum  */
    ret = rd_cmd_response_check_and_get_uword(rd_interface, payload_len + RD_PROTO_POS_BYTE_0, &response_checksum);
    if (ret < 0)
    {
        return ret;
    }
    /* validate checksum */
    checksum = rd_checksum(rd_interface->response.ptr, payload_len + RD_PROTO_POS_BYTE_0);
    if (response_checksum != checksum)
    {
        fprintf(stderr, "response checksum not match\n");
        return -011603;
    }
    /* extract status */
    ret = rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_0, &rd_interface->last_response_status);
    if (ret < 0)
    {
        return ret;
    }
    if (rd_interface->last_response_status != 0)
    {
        fprintf(stderr, "device returns failed status: 0x%X\n", rd_interface->last_response_status);
        return -011604;
    }
    return 0;
}

/* ================================================================== */
/* RdInterfaceInit */
RD_INTERFACE* RdInterfaceInit(const char* port_name)
{
    RD_INTERFACE* rd_interface = NULL;
    int ret;

    rd_interface = (RD_INTERFACE*) malloc(sizeof(RD_INTERFACE));
    memset(rd_interface, 0, sizeof(RD_INTERFACE));

    /* open serial port */
    ret = rd_extint_open(rd_interface, port_name);
    if (ret < 0)
    {
        free(rd_interface);
        return NULL;
    }

    /* prepare interface */
    rd_interface->is_open = 1;

    return rd_interface;
}

/* ================================================================== */
/* RdInterfaceClose */
int RdInterfaceClose(RD_INTERFACE* rd_interface)
{
    _RD_CHECK_INTERFACE();

    if (rd_interface->request.ptr)
    {
        free(rd_interface->request.ptr);
    }
    if (rd_interface->response.ptr)
    {
        free(rd_interface->response.ptr);
    }

    rd_extint_close(rd_interface);
    free(rd_interface);
    return 0;
}

/* ================================================================== */
/* RdFreeData */
int RdFreeData(void* data)
{
    if (data)
    {
        free(data);
    }
    data = NULL;
    return 0;
}

/* ================================================================== */
/* Rd_SetLayerEnable */
int Rd_SetLayerEnable(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_FLAG enable)
{
    int ret;

	//RD_DBG(1, "Rd_SetLayerEnable layer_id: %d enable: %d\n", layer_id, enable);
    ret = rd_cmd_request_init(rd_interface, Cmd_SetLayerEnable);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_flag(rd_interface, enable);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_SetLayerOriginAndSize */
int Rd_SetLayerOriginAndSize(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_POSITION position, RD_SIZE size)
{
    int ret;

	//RD_DBG(1, "Rd_SetLayerOriginAndSize layer_id: %d position: %dx%d, size: %dx%d\n", layer_id, position.x, position.y, size.width, size.height);
    ret = rd_cmd_request_init(rd_interface, Cmd_SetLayerOriginAndSize);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.height);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_SetLayerBackColor */
int Rd_SetLayerBackColor(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_COLOR back_color)
{
    int ret;

	//RD_DBG(1, "Rd_SetLayerBackColor layer_id: %d color: %02X %02X %02X %02X\n", layer_id, back_color.red, back_color.green, back_color.blue, back_color.alpha);
    ret = rd_cmd_request_init(rd_interface, Cmd_SetLayerBackColor);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_color(rd_interface, back_color);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_SetLayerTransparency */
int Rd_SetLayerTransparency(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_BYTE transparency_percentage)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_SetLayerTransparency);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, transparency_percentage);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LayerClear */
int Rd_LayerClear(RD_INTERFACE* rd_interface, RD_ID layer_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_LayerClear);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LayerMove */
int Rd_LayerMove(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_UWORD move_left, RD_UWORD move_top, RD_UWORD move_right, RD_UWORD move_bottom)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_LayerMove);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, move_left);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, move_top);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, move_right);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, move_bottom);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LayerWriteRawPixels */
int Rd_LayerWriteRawPixels(RD_INTERFACE* rd_interface, RD_ID layer_id,
RD_POSITION position, RD_SIZE pixel_size, const RD_COLOR* pixels)
{
    int ret;
    int pixels_len_in_bytes, pixel_len;
    ret = rd_cmd_request_init(rd_interface, Cmd_LayerWriteRawPixels);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, pixel_size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, pixel_size.height);
    if (ret < 0)
    {
        return ret;
    }
    pixel_len = pixel_size.height * pixel_size.width;
    ret = rd_cmd_request_append_uword(rd_interface, pixel_len);
    if (ret < 0)
    {
        return ret;
    }
    pixels_len_in_bytes = pixel_size.height * pixel_size.width * sizeof(RD_COLOR);
    ret = rd_buffer_check_and_allocate(&rd_interface->request, rd_interface->request.size + pixels_len_in_bytes);
    if (ret < 0)
    {
        return ret;
    }
    memcpy(rd_interface->request.ptr + rd_interface->request.size, pixels, pixels_len_in_bytes);
    rd_interface->request.size += pixels_len_in_bytes;

    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ComposeLayersToPage */
int Rd_ComposeLayersToPage(RD_INTERFACE* rd_interface, RD_ID page_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ComposeLayersToPage);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, page_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_PageToScreen */
int Rd_PageToScreen(RD_INTERFACE* rd_interface, RD_ID page_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_PageToScreen);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, page_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_PartialComposeLayersToScreen */
int Rd_PartialComposeLayersToScreen(RD_INTERFACE* rd_interface, RD_ID layer_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_PartialComposeLayersToScreen);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageLoad */
int Rd_ImageLoad(RD_INTERFACE* rd_interface, const char* image_label, RD_ID* image_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageLoad);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, image_label);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, image_id);
}

/* ================================================================== */
/* Rd_ImageRelease */
int Rd_ImageRelease(RD_INTERFACE* rd_interface, RD_ID image_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageRelease);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageWrite */
int Rd_ImageWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_ID image_id,
RD_POSITION position, RD_ID* image_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageWrite);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, image_write_id);
}

/* ================================================================== */
/* Rd_ImageDelete */
int Rd_ImageDelete(RD_INTERFACE* rd_interface, RD_ID image_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageMove */
int Rd_ImageMove(RD_INTERFACE* rd_interface, RD_ID image_write_id, RD_POSITION position)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageMove);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageListLoad */
int Rd_ImageListLoad(RD_INTERFACE* rd_interface, const char* prefix, RD_UWORD index_start,
RD_UWORD index_step,
RD_UWORD index_count, RD_ID* image_list_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageListLoad);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, prefix);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, index_start);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, index_step);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, index_count);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, image_list_id);
}

/* ================================================================== */
/* Rd_ImageListRelease */
int Rd_ImageListRelease(RD_INTERFACE* rd_interface, RD_ID image_list_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageListRelease);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_list_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageListWrite */
int Rd_ImageListWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID image_list_id, RD_UWORD image_index, RD_ID* image_list_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageListWrite);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_list_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_index);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, image_list_write_id);
}

/* ================================================================== */
/* Rd_ImageListReplace */
int Rd_ImageListReplace(RD_INTERFACE* rd_interface, RD_ID image_list_write_id, RD_UWORD image_index)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageListReplace);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_list_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_index);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_ImageListDelete */
int Rd_ImageListDelete(RD_INTERFACE* rd_interface, RD_ID image_list_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_ImageListDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_list_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_AnimationPlay */
int Rd_AnimationPlay(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID image_list_id, RD_UWORD frequency, RD_ID* animation_play_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_AnimationPlay);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_list_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, frequency);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, animation_play_id);
}

/* ================================================================== */
/* Rd_AnimationStop */
int Rd_AnimationStop(RD_INTERFACE* rd_interface, RD_ID animation_play_id, RD_UWORD stop_index)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_AnimationStop);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, animation_play_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, stop_index);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_AnimationContinue */
int Rd_AnimationContinue(RD_INTERFACE* rd_interface, RD_ID animation_play_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_AnimationContinue);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, animation_play_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_AnimationDelete */
int Rd_AnimationDelete(RD_INTERFACE* rd_interface, RD_ID animation_play_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_AnimationDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, animation_play_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_FontLoad */
int Rd_FontLoad(RD_INTERFACE* rd_interface, const char* font_label, RD_UWORD* font_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FontLoad);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, font_label);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, font_id);
}

/* ================================================================== */
/* Rd_FontRelease */
int Rd_FontRelease(RD_INTERFACE* rd_interface, RD_ID font_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FontRelease);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, font_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_SetFontPadding */
int Rd_SetFontPadding(RD_INTERFACE* rd_interface, RD_ID font_id, const RD_BYTE padding)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_SetFontPadding);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, font_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, padding);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_StringWrite */
int Rd_StringWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID font_id, RD_COLOR color, RD_HDIRECTION hdirection, const char* data, RD_ID* string_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_StringWrite);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, font_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_color(rd_interface, color);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_hdirection(rd_interface, hdirection);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, data);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, string_write_id);
}

/* ================================================================== */
/* Rd_StringReplace */
int Rd_StringReplace(RD_INTERFACE* rd_interface, RD_ID string_write_id, const char* data)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_StringReplace);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, string_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, data);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_StringDelete */
int Rd_StringDelete(RD_INTERFACE* rd_interface, RD_ID string_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_StringDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, string_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_CharacterWrite */
int Rd_CharacterWrite(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_ID font_id, RD_COLOR color, RD_BYTE c, RD_ID* character_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_CharacterWrite);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, font_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_color(rd_interface, color);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, c);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, character_write_id);
}

/* ================================================================== */
/* Rd_CharacterReplace */
int Rd_CharacterReplace(RD_INTERFACE* rd_interface, RD_ID character_write_id, RD_BYTE c)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_CharacterReplace);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, character_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, c);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_CharacterDelete */
int Rd_CharacterDelete(RD_INTERFACE* rd_interface, RD_ID character_write_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_CharacterDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, character_write_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TextWindowCreate */
int Rd_TextWindowCreate(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_SIZE size, RD_ID font_id, RD_COLOR fontcolor, RD_HDIRECTION scroll_direction, RD_UWORD* text_window_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TextWindowCreate);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.height);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, font_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_color(rd_interface, fontcolor);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_hdirection(rd_interface, scroll_direction);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, text_window_id);
}

/* ================================================================== */
/* Rd_TextWindowSetInsertionPoint */
int Rd_TextWindowSetInsertionPoint(RD_INTERFACE* rd_interface, RD_ID text_window_id, RD_POSITION position)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TextWindowSetInsertionPoint);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, text_window_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TextWindowInsertText */
int Rd_TextWindowInsertText(RD_INTERFACE* rd_interface, RD_ID text_window_id, const char* stringData)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TextWindowInsertText);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, text_window_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, stringData);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TextWindowDelete */
int Rd_TextWindowDelete(RD_INTERFACE* rd_interface, RD_ID text_window_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TextWindowDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, text_window_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LineGraphCreateWindow */
int Rd_LineGraphCreateWindow(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position,
RD_SIZE size, RD_BYTE line_width, RD_BYTE line_glow_width, RD_FLAG autocompose, RD_ID* graph_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_LineGraphCreateWindow);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.height);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, line_width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, line_glow_width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_flag(rd_interface, autocompose);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, graph_id);
}

/* ================================================================== */
/* Rd_LineGraphInsertPoints */
int Rd_LineGraphInsertPoints(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_COLOR point_color,
RD_UWORD point_length, const RD_POSITION* points)
{
    int ret;
    int points_len_in_bytes;
    ret = rd_cmd_request_init(rd_interface, Cmd_LineGraphInsertPoints);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_color(rd_interface, point_color);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, point_length);
    if (ret < 0)
    {
        return ret;
    }
    points_len_in_bytes = point_length * sizeof(RD_COLOR);
    ret = rd_buffer_check_and_allocate(&rd_interface->request, rd_interface->request.size + points_len_in_bytes);
    if (ret < 0)
    {
        return ret;
    }
    memcpy(rd_interface->request.ptr + rd_interface->request.size, points, points_len_in_bytes);
    rd_interface->request.size += points_len_in_bytes;

    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LineGraphMove */
int Rd_LineGraphMove(RD_INTERFACE* rd_interface, RD_ID graph_id,
RD_UWORD left, RD_UWORD top, RD_UWORD right, RD_UWORD bottom)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_LineGraphMove);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, left);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, top);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, right);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, bottom);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_LineGraphDeleteWindow */
int Rd_LineGraphDeleteWindow(RD_INTERFACE* rd_interface, RD_ID graph_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_LineGraphDeleteWindow);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_BarGraphCreateWindow */
int Rd_BarGraphCreateWindow(RD_INTERFACE* rd_interface, RD_ID layer_id, RD_POSITION position, RD_SIZE size,
RD_BYTE stack_size, RD_DIRECTION stack_direction, RD_FLAG autocompose, RD_ID* graph_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_BarGraphCreateWindow);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, layer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.height);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, stack_size);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, stack_direction);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_flag(rd_interface, autocompose);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, graph_id);
}

/* ================================================================== */
/* Rd_BarGraphInsertStacks */
int Rd_BarGraphInsertStacks(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_BYTE no_of_stack, RD_ID image_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_BarGraphInsertStacks);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, no_of_stack);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, image_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_BarGraphRemoveStacks */
int Rd_BarGraphRemoveStacks(RD_INTERFACE* rd_interface, RD_ID graph_id, RD_BYTE no_of_stack)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_BarGraphRemoveStacks);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_byte(rd_interface, no_of_stack);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_BarGraphDeleteWindow */
int Rd_BarGraphDeleteWindow(RD_INTERFACE* rd_interface, RD_ID graph_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_BarGraphDeleteWindow);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, graph_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TouchMapRectangle */
int Rd_TouchMapRectangle(RD_INTERFACE* rd_interface, RD_POSITION position, RD_SIZE size, const char* label, RD_ID* touch_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TouchMapRectangle);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.width);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, size.height);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, label);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, touch_id);
}

/* ================================================================== */
/* Rd_TouchMapCircle */
int Rd_TouchMapCircle(RD_INTERFACE* rd_interface, RD_POSITION position,
RD_UWORD outer_circle_radius, RD_UWORD inner_circle_radius, const char* label, RD_ID* touch_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TouchMapCircle);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.x);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, position.y);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, outer_circle_radius);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, inner_circle_radius);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, label);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, touch_id);
}

/* ================================================================== */
/* Rd_TouchMapDelete */
int Rd_TouchMapDelete(RD_INTERFACE* rd_interface, RD_ID touch_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TouchMapDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, touch_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TouchMapClear */
int Rd_TouchMapClear(RD_INTERFACE* rd_interface)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TouchMapClear);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_Reset */
int Rd_Reset(RD_INTERFACE* rd_interface)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_Reset);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_TestEcho */
int Rd_TestEcho(RD_INTERFACE* rd_interface, const char* label, char** output)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_TestEcho);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, label);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_data(rd_interface, 8, output);
}

/* ================================================================== */
/* Rd_SystemInfo */
int Rd_SystemInfo(RD_INTERFACE* rd_interface, RD_GET_VERSION_TYPE type, char** output)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_SystemInfo);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, type);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_data(rd_interface, 8, output);
}

/* ================================================================== */
/* Rd_EventMessage */
int Rd_EventMessage(RD_INTERFACE* rd_interface, RD_EVENT** event, RD_UWORD* count)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_EventMessage);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_event_data(rd_interface, event, 8, count);
}

/* ================================================================== */
/* Rd_FlashWriteEnable */
int Rd_FlashWriteEnable(RD_INTERFACE* rd_interface, RD_FLAG enable)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FlashWriteEnable);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_flag(rd_interface, enable);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_FlashImage */
int Rd_FlashImage(RD_INTERFACE* rd_interface, RD_UWORD type, const char* filename, RD_UWORD length, RD_ID* transfer_id)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FlashImage);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, type);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, filename);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, length);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, transfer_id);
}

/* ================================================================== */
/* Rd_FlashData */
int Rd_FlashData(RD_INTERFACE* rd_interface, RD_ID transfer_id, RD_UWORD type, const char* data)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FlashData);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, transfer_id);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, type);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, data);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_FlashDelete */
int Rd_FlashDelete(RD_INTERFACE* rd_interface, RD_UWORD type, const char* filename)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FlashDelete);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, type);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_string(rd_interface, filename);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_FlashDeleteAll */
int Rd_FlashDeleteAll(RD_INTERFACE* rd_interface)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_FlashDeleteAll);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}

/* ================================================================== */
/* Rd_GetMaxBackLightBrightness */
int Rd_GetMaxBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD* max_backlight_brightness)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_GetMaxBackLightBrightness);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, max_backlight_brightness);
}

/* ================================================================== */
/* Rd_GetBackLightBrightness */
int Rd_GetBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD* backlight_brightness)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_GetBackLightBrightness);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_response_receive(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_check_and_get_uword(rd_interface, RD_PROTO_POS_BYTE_1, backlight_brightness);
}

/* ================================================================== */
/* Rd_SetBackLightBrightness */
int Rd_SetBackLightBrightness(RD_INTERFACE* rd_interface, RD_UWORD backlight_brightness)
{
    int ret;
    ret = rd_cmd_request_init(rd_interface, Cmd_SetBackLightBrightness);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_append_uword(rd_interface, backlight_brightness);
    if (ret < 0)
    {
        return ret;
    }
    ret = rd_cmd_request_process(rd_interface);
    if (ret < 0)
    {
        return ret;
    }
    return rd_cmd_response_receive(rd_interface);
}


