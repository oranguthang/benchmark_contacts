#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <jansson.h>

#define PORT 8080

PGconn *conn;

static int
answer_to_connection(void *cls, struct MHD_Connection *connection,
                     const char *url, const char *method,
                     const char *version, const char *upload_data,
                     size_t *upload_data_size, void **con_cls) {
    if (strcmp(method, "GET") == 0 && strcmp(url, "/ping") == 0) {
        const char *page = "pong";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                                                        (void*)page,
                                                                        MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
    }

    if (strcmp(method, "POST") == 0 && strcmp(url, "/contacts") == 0) {
        if (*upload_data_size != 0) {
            // Парсим JSON
            json_error_t error;
            json_t *root = json_loads(upload_data, 0, &error);
            if (!root) {
                const char *err = "Invalid JSON";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err),
                                                                                (void*)err,
                                                                                MHD_RESPMEM_PERSISTENT);
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                *upload_data_size = 0;
                return ret;
            }

            const char *external_id = json_string_value(json_object_get(root, "external_id"));
            const char *phone = json_string_value(json_object_get(root, "phone_number"));

            if (!external_id || !phone) {
                json_decref(root);
                const char *err = "Missing fields";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err),
                                                                                (void*)err,
                                                                                MHD_RESPMEM_PERSISTENT);
                int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
                MHD_destroy_response(response);
                *upload_data_size = 0;
                return ret;
            }

            // Вставляем в БД
            char query[512];
            snprintf(query, sizeof(query),
                     "INSERT INTO contacts (external_id, phone_number) VALUES ('%s', '%s')",
                     external_id, phone);

            PGresult *res = PQexec(conn, query);
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                fprintf(stderr, "INSERT failed: %s\n", PQerrorMessage(conn));
                PQclear(res);
                json_decref(root);
                const char *err = "DB error";
                struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err),
                                                                                (void*)err,
                                                                                MHD_RESPMEM_PERSISTENT);
                int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
                MHD_destroy_response(response);
                *upload_data_size = 0;
                return ret;
            }

            PQclear(res);
            json_decref(root);

            const char *msg = "{\"status\":\"ok\"}";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(msg),
                                                                            (void*)msg,
                                                                            MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);
            MHD_destroy_response(response);
            *upload_data_size = 0;
            return ret;
        }

        // Обязательно сбрасывать размер перед возвращением
        *upload_data_size = 0;
        return MHD_YES;
    }

    const char *page = "Not found";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                                                    (void*)page,
                                                                    MHD_RESPMEM_PERSISTENT);
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
    while (1) {
        sleep(1);
    }

    MHD_stop_daemon(daemon);
    PQfinish(conn);

    return 0;
}
