use axum::{
    extract::{Query, State},
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use chrono::Utc;
use serde::{Deserialize, Serialize};
use sqlx::{postgres::PgPoolOptions, PgPool};
use std::net::SocketAddr;
use tokio::net::TcpListener;
use tower_http::trace::TraceLayer;
use tracing::Level;
use uuid::Uuid;

#[derive(Debug, Serialize)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: chrono::DateTime<Utc>,
    date_updated: chrono::DateTime<Utc>,
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
        .max_connections((num_cpus::get() * 4) as u32)
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
    let mut sql = String::from("SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE 1=1");
    let mut bindings: Vec<(String, sqlx::types::Type, Box<dyn sqlx::Encode<'_, sqlx::Postgres> + Send + Sync>)> = vec![];

    if let Some(ext_id) = params.external_id {
        sql.push_str(" AND external_id = $1");
        bindings.push((
            "external_id".into(),
            sqlx::types::Type::INT4,
            Box::new(ext_id),
        ));
    }

    if let Some(ref phone) = params.phone_number {
        sql.push_str(" AND phone_number ILIKE $2");
        bindings.push((
            "phone_number".into(),
            sqlx::types::Type::TEXT,
            Box::new(format!("%{}%", phone)),
        ));
    }

    sql.push_str(" ORDER BY date_created DESC");
    sql.push_str(&format!(
        " LIMIT {} OFFSET {}",
        params.limit.unwrap_or(100),
        params.offset.unwrap_or(0)
    ));

    let contacts = sqlx::query_as::<_, Contact>(&sql)
        .fetch_all(&state.db_pool)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(contacts))
}
