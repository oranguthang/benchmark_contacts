table! {
    contacts (id) {
        id -> Uuid,
        external_id -> Int4,
        phone_number -> Varchar,
        date_created -> Timestamp,
        date_updated -> Timestamp,
    }
}
