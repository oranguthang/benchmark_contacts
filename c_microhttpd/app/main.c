#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <jansson.h>

#define PORT 8080

typedef struct {
    char *post_data;
    size_t size;
} ConnectionInfo;

PGconn* get_pg_connection() {
    const char *conninfo = "host=db dbname=contacts_db user=user password=password";
    PGconn *conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "DB connect error: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return NULL;
    }
    return conn;
}

// GET /contacts
static enum MHD_Result handle_get_contacts(struct MHD_Connection *connection,
                                           const char *url,
                                           const char *method,
                                           const char *version,
                                           const char *upload_data,
                                           size_t *upload_data_size,
                                           void **con_cls) {
    PGconn *conn = get_pg_connection();
    if (!conn) {
        const char *err = "DB connection error";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        return ret;
    }

    const char *external_id = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "external_id");
    const char *phone_number = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "phone_number");
    const char *limit = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "limit");
    const char *offset = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "offset");

    char query[1024] = "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE 1=1";

    if (external_id)
        snprintf(query + strlen(query), sizeof(query) - strlen(query), " AND external_id = %s", external_id);
    if (phone_number)
        snprintf(query + strlen(query), sizeof(query) - strlen(query), " AND phone_number = '%s'", phone_number);
    if (limit)
        snprintf(query + strlen(query), sizeof(query) - strlen(query), " LIMIT %s", limit);
    if (offset)
        snprintf(query + strlen(query), sizeof(query) - strlen(query), " OFFSET %s", offset);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        const char *err = "DB query error";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        PQclear(res);
        PQfinish(conn);
        return ret;
    }

    json_t *contacts = json_array();
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        json_t *contact = json_object();
        json_object_set_new(contact, "id", json_string(PQgetvalue(res, i, 0)));
        json_object_set_new(contact, "external_id", json_integer(atoi(PQgetvalue(res, i, 1))));
        json_object_set_new(contact, "phone_number", json_string(PQgetvalue(res, i, 2)));
        json_object_set_new(contact, "date_created", json_string(PQgetvalue(res, i, 3)));
        json_object_set_new(contact, "date_updated", json_string(PQgetvalue(res, i, 4)));
        json_array_append_new(contacts, contact);
    }

    char *resp_str = json_dumps(contacts, 0);
    json_decref(contacts);
    PQclear(res);
    PQfinish(conn);

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(resp_str), resp_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

// GET /ping
static enum MHD_Result handle_get_ping(struct MHD_Connection *connection) {
    const char *page = "pong";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "text/plain");
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return ret;
}

// POST /contacts
static enum MHD_Result handle_post_contacts(struct MHD_Connection *connection,
                                            const char *upload_data,
                                            size_t *upload_data_size,
                                            void **con_cls) {
    ConnectionInfo *con_info = *con_cls;

    if (*upload_data_size != 0) {
        con_info->post_data = realloc(con_info->post_data, con_info->size + *upload_data_size + 1);
        memcpy(con_info->post_data + con_info->size, upload_data, *upload_data_size);
        con_info->size += *upload_data_size;
        con_info->post_data[con_info->size] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    json_error_t error;
    json_t *root = json_loads(con_info->post_data, 0, &error);
    if (!root) {
        const char *err = "Invalid JSON";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        goto cleanup;
    }

    json_t *external_id_json = json_object_get(root, "external_id");
    json_t *phone_json = json_object_get(root, "phone_number");
    if (!json_is_integer(external_id_json) || !json_is_string(phone_json)) {
        const char *err = "Missing/invalid fields";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
        MHD_destroy_response(response);
        json_decref(root);
        goto cleanup;
    }

    int external_id = json_integer_value(external_id_json);
    const char *phone_number = json_string_value(phone_json);

    PGconn *conn = get_pg_connection();
    if (!conn) {
        const char *err = "DB connection error";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        json_decref(root);
        goto cleanup;
    }

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO contacts (external_id, phone_number) VALUES (%d, '%s') RETURNING id, date_created, date_updated",
             external_id, phone_number);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        const char *err = "DB error";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(err), (void*)err, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "text/plain");
        int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
        MHD_destroy_response(response);
        PQclear(res);
        PQfinish(conn);
        json_decref(root);
        goto cleanup;
    }

    json_t *resp = json_object();
    json_object_set_new(resp, "id", json_string(PQgetvalue(res, 0, 0)));
    json_object_set_new(resp, "external_id", json_integer(external_id));
    json_object_set_new(resp, "phone_number", json_string(phone_number));
    json_object_set_new(resp, "date_created", json_string(PQgetvalue(res, 0, 1)));
    json_object_set_new(resp, "date_updated", json_string(PQgetvalue(res, 0, 2)));

    char *resp_str = json_dumps(resp, 0);
    json_decref(resp);
    PQclear(res);
    PQfinish(conn);
    json_decref(root);

    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(resp_str), resp_str, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    int ret = MHD_queue_response(connection, MHD_HTTP_CREATED, response);
    MHD_destroy_response(response);

cleanup:
    free(con_info->post_data);
    free(con_info);
    *con_cls = NULL;
    return MHD_YES;
}

enum MHD_Result answer_to_connection(void *cls, struct MHD_Connection *connection,
                                     const char *url, const char *method,
                                     const char *version, const char *upload_data,
                                     size_t *upload_data_size, void **con_cls) {
    if (*con_cls == NULL) {
        ConnectionInfo *con_info = calloc(1, sizeof(ConnectionInfo));
        *con_cls = con_info;
        return MHD_YES;
    }

    if (strcmp(method, "GET") == 0 && strcmp(url, "/ping") == 0)
        return handle_get_ping(connection);

    if (strcmp(method, "POST") == 0 && strcmp(url, "/contacts") == 0)
        return handle_post_contacts(connection, upload_data, upload_data_size, con_cls);

    if (strcmp(method, "GET") == 0 && strcmp(url, "/contacts") == 0)
        return handle_get_contacts(connection, url, method, version, upload_data, upload_data_size, con_cls);

    const char *page = "Not found";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(page), (void*)page, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(response, "Content-Type", "text/plain");
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return ret;
}

int main() {
    struct MHD_Daemon *daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                                                 &answer_to_connection, NULL, MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    printf("Server running on port %d\n", PORT);
    pause();
    MHD_stop_daemon(daemon);
    return 0;
}
