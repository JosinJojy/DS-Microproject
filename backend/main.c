/*
 * main.c — MazeLab HTTP server entry point
 *
 * Usage:  mazelab [port]
 *   port: optional, default 8080
 *
 * The server binds to 127.0.0.1 (loopback) only.
 * Static frontend files are served from the ../frontend directory
 * relative to the executable's working directory.
 */

#define MG_ENABLE_LINES 1
#include "../third_party/mongoose/mongoose.h"
#include "api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static int s_signo = 0;
static void signal_handler(int signo) { s_signo = signo; }

/* Mongoose event handler */
static void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    const char *frontend_root = (const char *)c->fn_data;
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        api_dispatch(c, hm, frontend_root);
    }
}

int main(int argc, char *argv[]) {
    /* Determine port */
    const char *port = "8080";
    if (argc >= 2) port = argv[1];

    /* Construct listen address — loopback only */
    char listen_addr[64];
    snprintf(listen_addr, sizeof(listen_addr), "http://127.0.0.1:%s", port);

    /* Frontend root: relative to CWD (run from project root) */
    const char *frontend_root = "frontend";

    /* Install signal handlers for clean shutdown */
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialise Mongoose event manager */
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    /* Start listening */
    struct mg_connection *nc =
        mg_http_listen(&mgr, listen_addr, ev_handler, (void *)frontend_root);

    if (nc == NULL) {
        fprintf(stderr,
                "[MazeLab] ERROR: Cannot bind to %s\n"
                "  Check that port %s is not already in use.\n"
                "  Run:  mazelab <port>  to use a different port.\n",
                listen_addr, port);
        mg_mgr_free(&mgr);
        return 1;
    }

    printf("============================================================\n");
    printf("  MazeLab v1.0.0 — Interactive Maze Solver\n");
    printf("============================================================\n");
    printf("  Server listening on : %s\n", listen_addr);
    printf("  Open in browser     : http://127.0.0.1:%s\n", port);
    printf("  Frontend root       : %s\n", frontend_root);
    printf("  Press Ctrl+C to stop.\n");
    printf("============================================================\n");
    fflush(stdout);

    /* Event loop */
    while (s_signo == 0) {
        mg_mgr_poll(&mgr, 100); /* poll with 100ms timeout */
    }

    printf("\n[MazeLab] Received signal %d, shutting down...\n", s_signo);
    mg_mgr_free(&mgr);
    printf("[MazeLab] Goodbye.\n");
    return 0;
}
