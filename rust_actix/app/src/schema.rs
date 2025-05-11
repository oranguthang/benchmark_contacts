table! {
    contacts (id) {
        id -> Int4,
        external_id -> Int4,
        phone_number -> Varchar,
        date_created -> Timestamptz,
        date_updated -> Timestamptz,
    }
}
