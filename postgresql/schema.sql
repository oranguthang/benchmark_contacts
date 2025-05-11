CREATE EXTENSION IF NOT EXISTS "pgcrypto";

CREATE TABLE IF NOT EXISTS contacts (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    external_id INT NOT NULL,
    phone_number TEXT NOT NULL,
    date_created TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    date_updated TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_contacts_external_id ON contacts(external_id);

CREATE INDEX IF NOT EXISTS idx_contacts_id ON contacts(id);
