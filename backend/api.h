#ifndef API_H
#define API_H

/* Forward-declare mongoose connection type to avoid including mongoose.h
 * in every translation unit that includes api.h. */
struct mg_connection;
struct mg_http_message;

/**
 * Dispatch an incoming HTTP message to the appropriate handler.
 * This is called from the Mongoose event handler in main.c.
 */
void api_dispatch(struct mg_connection *c, struct mg_http_message *hm,
                  const char *frontend_root);

#endif /* API_H */
