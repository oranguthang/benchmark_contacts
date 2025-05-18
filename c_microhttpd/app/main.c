#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include <jansson.h>

#define PORT 8080

PGconn *conn;

typedef struct {
    char *post_data;
    size_t size;
} ConnectionInfo;

static int
answer_to_connection(void *cls, struct MHD_Connection *connection,
                     const char *url, const char *method,
                     const char *version, const char *upload_data,
                     size_t *upload_data_size, void **con_cls) {
    if (*con_cls == NULL) {
        ConnectionInfo *con_info = calloc(1, sizeof(ConnectionInfo));
        *con_cls = (void *)con_info;
        return MHD_YES;
    }

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
        ConnectionInfo *con_info = *con_cls;

        if (*upload_data_size != 0) {
            // накапливаем данные
            con_info->post_data = realloc(con_info->post_data, con_info->size + *upload_data_size + 1);
            memcpy(con_info->post_data + con_info->size, upload_data, *upload_data_size);
            con_info->size += *upload_data_size;
            con_info->post_data[con_info->size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }

        // данные получены полностью — разбираем JSON
        json_error_t error;
        json_t *root = json_loads(con_info->post_data, 0, &error);
        if (!root) {
            const char *err = "Invalid JSON";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            goto cleanup;
        }

        json_t *external_id_json = json_object_get(root, "external_id");
        json_t *phone_json = json_object_get(root, "phone_number");

        if (!json_is_integer(external_id_json) || !json_is_string(phone_json)) {
            const char *err = "Missing or invalid fields";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            json_decref(root);
            goto cleanup;
        }

        int external_id = json_integer_value(external_id_json);
        const char *phone_number = json_string_value(phone_json);

        char query[512];
        snprintf(query, sizeof(query),
                 "INSERT INTO contacts (external_id, phone_number) VALUES (%d, '%s')",
                 external_id, phone_number);

        PGresult *res = PQexec(conn, query);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "DB Error: %s\n", PQerrorMessage(conn));
            const char *err = "DB error";
            struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
            int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            PQclear(res);
            json_decref(root);
            goto cleanup;
        }

        PQclear(res);
        json_decref(root);

        const char *ok = "{\"status\":\"ok\"}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(ok), (void*)ok, MHD_RESPMEM_PERSISTENT);
        int ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);
        MHD_destroy_response(response);

    cleanup:
        free(con_info->post_data);
        free(con_info);
        *con_cls = NULL;
        return MHD_YES;
    }

    const char *page = "Not found";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page),
                                                                    (void*)page,
                                                                    MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;

    return MHD_NO;
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
