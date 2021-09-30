/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-22     zhenzhen1003       the first version
 */
#include <string.h>

#include <rtthread.h>
#include <webclient.h>
#include "onenet_http.h"

#define POST_RESP_BUFSZ                1024
#define POST_HEADER_BUFSZ              1024

#define POST_LOCAL_URI_SOURCE                    "http://api.heclouds.com/bindata?device_id=789525469&datastream_id=bin&desc=testfile"
#define POST_LOCAL_URI_DETECTION                 "http://api.heclouds.com/bindata?device_id=789525469&datastream_id=bin2&desc=testfile"


/* send HTTP POST request by common request interface, it used to receive longer data */
static int webclient_post_comm(const char *uri, const char *post_data)
{
    struct webclient_session* session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;

    buffer = (unsigned char *) web_malloc(POST_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        rt_kprintf("no memory for receive response buffer.\n");
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* create webclient session and set header response size */
    session = webclient_session_create(POST_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* build header for upload */
    webclient_header_fields_add(session, "api-key: 1fS1DI30CNsE9i8h06SBJqi3VJU=\r\n");
    webclient_header_fields_add(session, "Host:api.heclouds.com\r\n");
    webclient_header_fields_add(session, "Connection:close\r\n");
    webclient_header_fields_add(session, "Content-Length:172856\r\n");//,rt_strlen(post_data)); 240*240µÄbmp=172856


    /* send POST request by default header */
    if ((resp_status = webclient_post(session, uri, post_data)) != 200)
    {
        rt_kprintf("webclient POST request failed, response(%d) error.\n", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }

    rt_kprintf("webclient post response data: \n");
    do
    {
        bytes_read = webclient_read(session, buffer, POST_RESP_BUFSZ);
        if (bytes_read <= 0)
        {
            break;
        }

        for (index = 0; index < bytes_read; index++)
        {
            rt_kprintf("%c", buffer[index]);
        }
    } while (1);

    rt_kprintf("\n");

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    return ret;
}



int webclient_post_test(unsigned char* post_data, int mode)
{
    char *uri = RT_NULL;

    if(mode == 1){
        uri = web_strdup(POST_LOCAL_URI_SOURCE);
    }
    else {
        uri = web_strdup(POST_LOCAL_URI_DETECTION);
    }
    if(uri == RT_NULL)
    {
        rt_kprintf("no memory for create post request uri buffer.\n");
        return -RT_ENOMEM;
    }

    webclient_post_comm(uri, post_data);

    if (uri)
    {
        web_free(uri);
    }

    return RT_EOK;
}


#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(webclient_post_test, web_post_test, webclient post request test.);
#endif /* FINSH_USING_MSH */
