use axum::{
    extract::{Query, State},
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use serde::{Deserialize, Serialize};
use sqlx::{
    postgres::PgPoolOptions,
    types::chrono::{DateTime, Utc},
    FromRow, PgPool, Row,
};
use std::net::SocketAddr;
use tokio::net::TcpListener;
use tower_http::trace::TraceLayer;
use tracing::Level;
use uuid::Uuid;
use sqlx::postgres::PgArguments;
use sqlx::Arguments;

#[derive(Debug, Serialize)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: DateTime<Utc>,
    date_updated: DateTime<Utc>,
}

impl<'r> FromRow<'r, sqlx::postgres::PgRow> for Contact {
    fn from_row(row: &'r sqlx::postgres::PgRow) -> Result<Self, sqlx::Error> {
        Ok(Contact {
            id: row.get("id"),
            external_id: row.get("external_id"),
            phone_number: row.get("phone_number"),
            date_created: row.get("date_created"),
            date_updated: row.get("date_updated"),
        })
    }
}

#[derive(Debug, Deserialize)]
struct ContactCreate {
    external_id: i32,
    phone_number: String,
}

#[derive(Debug, Deserialize)]
struct QueryParams {
    external_id: Option<i32>,
    phone_number: Option<String>,
    limit: Option<i64>,
    offset: Option<i64>,
}

#[derive(Clone)]
struct AppState {
    db_pool: PgPool,
}

fn main() -> anyhow::Result<()> {
    let runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(num_cpus::get())
        .max_blocking_threads(num_cpus::get() * 2)
        .enable_all()
        .build()?;

    runtime.block_on(async_main())
}

async fn async_main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_max_level(Level::INFO)
        .init();

    let db_pool = PgPoolOptions::new()
        .max_connections(num_cpus::get() as u32 * 4)
        .connect(&std::env::var("DATABASE_URL")?)
        .await?;

    let app_state = AppState { db_pool };

    let app = Router::new()
        .route("/ping", get(ping))
        .route("/contacts", post(create_contact).get(list_contacts))
        .with_state(app_state)
        .layer(TraceLayer::new_for_http());

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    let listener = TcpListener::bind(addr).await?;
    tracing::info!("Server started on {}", addr);

    axum::serve(listener, app).await?;
    Ok(())
}

async fn ping() -> &'static str {
    "pong"
}

async fn create_contact(
    State(state): State<AppState>,
    Json(payload): Json<ContactCreate>,
) -> Result<(StatusCode, Json<Contact>), (StatusCode, String)> {
    let row = sqlx::query(
        r#"
        INSERT INTO contacts (external_id, phone_number)
        VALUES ($1, $2)
        RETURNING id, external_id, phone_number, date_created, date_updated
        "#,
    )
    .bind(payload.external_id)
    .bind(payload.phone_number)
    .fetch_one(&state.db_pool)
    .await
    .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let contact = Contact {
        id: row.get("id"),
        external_id: row.get("external_id"),
        phone_number: row.get("phone_number"),
        date_created: row.get("date_created"),
        date_updated: row.get("date_updated"),
    };

    Ok((StatusCode::CREATED, Json(contact)))
}

async fn list_contacts(
    State(state): State<AppState>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (StatusCode, String)> {
    let mut query = "
        SELECT id, external_id, phone_number, date_created, date_updated
        FROM contacts
        WHERE TRUE
    ".to_string();

    let mut args = PgArguments::default();
    let mut arg_idx = 1;

    if let Some(external_id) = params.external_id {
        query.push_str(&format!(" AND external_id = ${}", arg_idx));
        args.add(external_id);
        arg_idx += 1;
    }

    if let Some(phone_number) = params.phone_number {
        query.push_str(&format!(" AND phone_number = ${}", arg_idx));
        args.add(phone_number);
        arg_idx += 1;
    }

    let limit = params.limit.unwrap_or(10000).min(10000);
    let offset = params.offset.unwrap_or(0);

    query.push_str(&format!(" LIMIT {} OFFSET {}", limit, offset));

    let mut query_builder = sqlx::query_as::<_, Contact>(&query);

    for arg in args {
        query_builder = query_builder.bind(arg);
    }

    let rows = query_builder
        .fetch_all(&state.db_pool)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(rows))
}
