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
    PgPool
};
use std::net::SocketAddr;
use tokio::net::TcpListener;
use tower_http::trace::TraceLayer;
use tracing::Level;
use uuid::Uuid;
use sqlx::QueryBuilder;

#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

#[derive(Debug, Serialize, sqlx::FromRow)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: DateTime<Utc>,
    date_updated: DateTime<Utc>,
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
    let worker_threads = std::env::var("CPU_CORES")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or_else(num_cpus::get);

    let runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(worker_threads + 1)
        .max_blocking_threads((worker_threads + 1) * 2)
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
    let contact = sqlx::query_as!(
        Contact,
        r#"
        INSERT INTO contacts (external_id, phone_number)
        VALUES ($1, $2)
        RETURNING id, external_id, phone_number, date_created, date_updated
        "#,
        payload.external_id,
        payload.phone_number
    )
    .fetch_one(&state.db_pool)
    .await
    .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok((StatusCode::CREATED, Json(contact)))
}


async fn list_contacts(
    State(state): State<AppState>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (StatusCode, String)> {
    let mut builder = QueryBuilder::new(
        "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE TRUE"
    );

    if let Some(external_id) = params.external_id {
        builder.push(" AND external_id = ").push_bind(external_id);
    }

    if let Some(ref phone_number) = params.phone_number {
        builder.push(" AND phone_number = ").push_bind(phone_number);
    }

    let limit = params.limit.unwrap_or(10000).min(10000);
    let offset = params.offset.unwrap_or(0);

    builder.push(" LIMIT ").push_bind(limit);
    builder.push(" OFFSET ").push_bind(offset);

    let query = builder.build_query_as::<Contact>();

    let contacts = query
        .fetch_all(&state.db_pool)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(contacts))
}
