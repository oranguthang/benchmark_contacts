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
use tower_http::trace::{DefaultMakeSpan, TraceLayer};
use tracing::{info, Level};
use uuid::Uuid;

mod schema;

type DbPool = deadpool_diesel::postgres::Pool;
type DbResult<T> = Result<T, (StatusCode, String)>;

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
async fn main() {
    tracing_subscriber::fmt()
        .with_max_level(Level::INFO)
        .init();

    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");

    let pool = create_pool(&database_url)
        .expect("Failed to create pool");

    let app = Router::new()
        .route("/ping", get(|| async { "pong" }))
        .route("/contacts",
            post(create_contact)
            .get(list_contacts)
        )
        .with_state(Arc::new(AppState { pool }))
        .layer(
            TraceLayer::new_for_http()
                .make_span_with(DefaultMakeSpan::new().include_headers(false))
        );

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    info!("Listening on {}", addr);

    axum::Server::bind(&addr)
        .serve(app.into_make_service())
        .await
        .unwrap();
}

async fn create_contact(
    State(state): State<Arc<AppState>>,
    Json(payload): Json<ContactCreate>,
) -> DbResult<(StatusCode, Json<Contact>)> {
    use schema::contacts::dsl::*;

    let conn = state.pool.get().await.map_err(internal_error)?;

    let contact = conn.interact(move |conn| {
        insert_into(contacts)
            .values(NewContact {
                id: Uuid::new_v4(),
                external_id: payload.external_id,
                phone_number: payload.phone_number,
                date_created: Utc::now(),
                date_updated: Utc::now(),
            })
            .get_result::<Contact>(conn)
    })
    .await
    .map_err(internal_error)?
    .map_err(internal_error)?;

    Ok((StatusCode::CREATED, Json(contact)))
}

async fn list_contacts(
    State(state): State<Arc<AppState>>,
    Query(params): Query<QueryParams>,
) -> DbResult<Json<Vec<Contact>>> {
    use schema::contacts::dsl::*;

    let conn = state.pool.get().await.map_err(internal_error)?;

    let contacts = conn.interact(move |conn| {
        let mut query = contacts.into_boxed();

        if let Some(ext_id) = params.external_id {
            query = query.filter(external_id.eq(ext_id));
        }

        if let Some(phone) = params.phone_number {
            query = query.filter(phone_number.like(format!("%{phone}%")));
        }

        query
            .limit(params.limit.unwrap_or(100))
            .offset(params.offset.unwrap_or(0))
            .load::<Contact>(conn)
    })
    .await
    .map_err(internal_error)?
    .map_err(internal_error)?;

    Ok(Json(contacts))
}

fn create_pool(database_url: &str) -> Result<DbPool, Box<dyn std::error::Error>> {
    let manager = deadpool_diesel::postgres::Manager::new(
        database_url,
        deadpool_diesel::Runtime::Tokio1,
    );
    Ok(deadpool_diesel::postgres::Pool::builder(manager).build()?)
}

fn internal_error<E: std::fmt::Display>(err: E) -> (StatusCode, String) {
    (StatusCode::INTERNAL_SERVER_ERROR, err.to_string())
}
