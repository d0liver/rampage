#ifndef LWS_SHORT_H

#include <libwebsockets.h>

typedef struct libwebsocket_context lws_ctx;
typedef struct libwebsocket lws_wsi;
typedef struct libwebsocket_protocols lws_protocols;
typedef enum libwebsocket_callback_reasons lws_callback_reasons;

#define lws_callback_on_writeable libwebsocket_callback_on_writable
#define lws_remaining_packet_payload libwebsockets_remaining_packet_payload 
#define lws_is_final_fragment libwebsocket_is_final_fragment 
#define lws_service libwebsocket_service 
#define lws_get_internal_extensions libwebsocket_get_internal_extensions 
#define lws_create_context libwebsocket_create_context 
#define lws_context_destroy libwebsocket_context_destroy
#define lws_write libwebsocket_write 

#define LWS_SHORT_H
#endif
