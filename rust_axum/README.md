    docker-compose up -d db
    docker-compose exec db pg_isready -U user

    sudo apt install cargo
    sudo apt install rustup
    rustup update stable
    DATABASE_URL=postgres://user:password@localhost:5432/contacts_db cargo install sqlx-cli --no-default-features --features postgres
    DATABASE_URL=postgres://user:password@localhost:5432/contacts_db cargo sqlx prepare
