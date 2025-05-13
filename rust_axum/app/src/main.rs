use axum::{
    extract::{Query, State},
    http::StatusCode,
    response::IntoResponse,
    routing::{get, post},
    Json, Router,
};
use chrono::Utc;
use deadpool_diesel::postgres::Pool;
use diesel::{prelude::*, insert_into};
use serde::{Deserialize, Serialize};
use std::{net::SocketAddr, sync::Arc};
use tower_http::{
    compression::CompressionLayer,
    trace::{DefaultMakeSpan, TraceLayer},
};
use tracing::{info, Level};
use uuid::Uuid;

mod schema;

// Типы для базы данных
type DbPool = deadpool_diesel::postgres::Pool;
type DbError = Box<dyn std::error::Error + Send + Sync>;

#[derive(Debug, Serialize, Queryable)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: chrono::DateTime<Utc>,
    date_updated: chrono::DateTime<Utc>,
}

#[derive(Debug, Deserialize, Insertable)]
#[diesel(table_name = schema::contacts)]
struct NewContact {
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
    pool: DbPool,
}

#[tokio::main]
async fn main() -> Result<(), DbError> {
    tracing_subscriber::fmt()
        .with_max_level(Level::INFO)
        .init();

    dotenv::dotenv().ok();
    let database_url = std::env::var("DATABASE_URL")?;

    let pool = create_pool(&database_url)?;
    let app_state = Arc::new(AppState { pool });

    let app = Router::new()
        .route("/ping", get(ping_handler))
        .route("/contacts",
            post(create_contact)
            .get(list_contacts)
        )
        .with_state(app_state)
        .layer(
            TraceLayer::new_for_http()
                .make_span_with(DefaultMakeSpan::new().include_headers(true))
        )
        .layer(CompressionLayer::new());

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    info!("Server listening on {}", addr);

    axum::Server::bind(&addr)
        .serve(app.into_make_service())
        .await?;
    Ok(())
}

async fn ping_handler() -> &'static str {
    "pong"
}

async fn create_contact(
    State(state): State<Arc<AppState>>,
    Json(payload): Json<ContactCreate>,
) -> Result<(StatusCode, Json<Contact>), (StatusCode, String)> {
    use schema::contacts::dsl::*;

    let conn = state.pool.get().await.map_err(internal_error)?;

    let new_contact = NewContact {
        id: Uuid::new_v4(),
        external_id: payload.external_id,
        phone_number: payload.phone_number,
        date_created: Utc::now(),
        date_updated: Utc::now(),
    };

    conn.interact(|conn| {
        insert_into(contacts)
            .values(&new_contact)
            .get_result::<Contact>(conn)
    })
    .await
    .map_err(internal_error)?
    .map(|contact| (StatusCode::CREATED, Json(contact)))
    .map_err(internal_error)
}

async fn list_contacts(
    State(state): State<Arc<AppState>>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (StatusCode, String)> {
    use schema::contacts::dsl::*;

    let conn = state.pool.get().await.map_err(internal_error)?;

    let mut query = contacts.into_boxed();

    if let Some(ext_id) = params.external_id {
        query = query.filter(external_id.eq(ext_id));
    }

    if let Some(ref phone) = params.phone_number {
        query = query.filter(phone_number.like(format!("%{}%", phone)));
    }

    let limit_val = params.limit.unwrap_or(100);
    let offset_val = params.offset.unwrap_or(0);

    conn.interact(move |conn| {
        query
            .limit(limit_val)
            .offset(offset_val)
            .load::<Contact>(conn)
    })
    .await
    .map_err(internal_error)?
    .map(Json)
    .map_err(internal_error)
}

fn create_pool(database_url: &str) -> Result<DbPool, DbError> {
    let manager = deadpool_diesel::postgres::Manager::new(
        database_url,
        deadpool_diesel::Runtime::Tokio1,
    );
    Ok(deadpool_diesel::postgres::Pool::builder(manager).build()?)
}

fn internal_error<E: std::fmt::Display>(err: E) -> (StatusCode, String) {
    (StatusCode::INTERNAL_SERVER_ERROR, err.to_string())
}
