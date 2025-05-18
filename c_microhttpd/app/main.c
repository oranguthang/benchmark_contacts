#include <microhttpd.h>
#include <libpq-fe.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8080

static PGconn *conn = NULL;

static int
answer_to_connection(void *cls, struct MHD_Connection *connection,
                     const char *url, const char *method,
                     const char *version, const char *upload_data,
                     size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "GET") != 0) {
        return MHD_NO; // only GET for simplicity
    }

    if (strcmp(url, "/ping") == 0) {
        const char *page = "pong";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                             (void*)page, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (strcmp(url, "/contacts") == 0) {
        PGresult *res = PQexec(conn, "SELECT id, external_id, phone_number FROM contacts LIMIT 100");
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            const char *err = "DB query error";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err),
                                                 (void*)err, MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            return ret;
        }

        int nrows = PQntuples(res);

        json_t *contacts = json_array();

        for (int i = 0; i < nrows; i++) {
            json_t *contact = json_object();
            json_object_set_new(contact, "id", json_string(PQgetvalue(res, i, 0)));
            json_object_set_new(contact, "external_id", json_integer(atoi(PQgetvalue(res, i, 1))));
            json_object_set_new(contact, "phone_number", json_string(PQgetvalue(res, i, 2)));
            json_array_append_new(contacts, contact);
        }

        PQclear(res);

        char *response_str = json_dumps(contacts, 0);
        json_decref(contacts);

        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_str),
                                             (void*)response_str, MHD_RESPMEM_MUST_FREE);

        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);

        return ret;
    }

    const char *notfound = "Not found";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(notfound),
                                         (void*)notfound, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    conn = PQconnectdb("user=user password=password host=db port=5432 dbname=contacts_db");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                              &answer_to_connection, NULL, MHD_OPTION_END);
    if (NULL == daemon) {
        fprintf(stderr, "Failed to start HTTP daemon\n");
        PQfinish(conn);
        return 1;
    }

    printf("Server running on port %d\n", PORT);
    getchar(); // wait for user input to stop server

    MHD_stop_daemon(daemon);
    PQfinish(conn);

    return 0;
}
