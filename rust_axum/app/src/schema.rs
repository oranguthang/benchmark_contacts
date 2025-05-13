diesel::table! {
    contacts (id) {
        id -> Uuid,
        external_id -> Int4,
        phone_number -> Text,
        date_created -> Timestamptz,
        date_updated -> Timestamptz,
    }
}
