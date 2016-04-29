/* libwebsockets gave us a write callback. Time to try to write out some data */
int writeable(struct lwss *wsi, struct Session *session)
{
	char *response_buff;
	int bytes_written;

	/* If the response was NULL or empty then we need to bail out here */
	if (!session->response) return 0;
	else session->response_len = strlen(session->response);
	if (!session->response_len) return 0;

	response_buff = malloc(LWS_SEND_BUFFER_PRE_PADDING + session->response_len);
	memcpy(
		response_buff + LWS_SEND_BUFFER_PRE_PADDING,
		session->response, session->response_len
	);

	debug("Response: %s\n", session->response);

	bytes_written = lwss_write (
		wsi,
		(unsigned char *)response_buff +
		LWS_SEND_BUFFER_PRE_PADDING,
		session->response_len,
		LWS_WRITE_TEXT
	);

	if (bytes_written < 0) {
		lwsl_err("ERROR %d writing to socket\n", bytes_written);
		return -1;
	}

	if (bytes_written < session->response_len)
		lwsl_err(
			"partial write %d vs %d\n",
			bytes_written, session->response_len
		);

	session->response_len = 0;
	session->response = NULL;

	if (lws_partial_buffered(wsi) || lws_send_pipe_choked(wsi)) {
		lwss_callback_on_writable(context, wsi);
		return 0;
	}

	/*
	 * For tests with chrome on same machine as client and server,
	 * this is needed to stop chrome choking.
	 */
	usleep(1);
}
