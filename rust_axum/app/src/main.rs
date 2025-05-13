use axum::{
    extract::{Query, State},
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use chrono::Utc;
use diesel::{prelude::*, insert_into};
use diesel_async::{pooled_connection::bb8::Pool, AsyncPgConnection, RunQueryDsl};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;
use tokio::net::TcpListener;
use tower_http::trace::{DefaultMakeSpan, TraceLayer};
use tracing::{info, Level};
use uuid::Uuid;

mod schema;

type DbPool = Pool<AsyncPgConnection>;

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

    dotenv::dotenv().ok();
    let database_url = std::env::var("DATABASE_URL")
        .expect("DATABASE_URL must be set");

    let manager = diesel_async::pooled_connection::AsyncDieselConnectionManager::<AsyncPgConnection>::new(database_url);
    let pool = Pool::builder()
        .build(manager)
        .await
        .expect("Failed to create pool");

    let app_state = AppState { pool };

    let app = Router::new()
        .route("/ping", get(|| async { "pong" }))
        .route("/contacts",
            post(create_contact)
            .get(list_contacts)
        )
        .with_state(app_state)
        .layer(
            TraceLayer::new_for_http()
                .make_span_with(DefaultMakeSpan::new().include_headers(false))
        );

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    let listener = TcpListener::bind(addr).await.unwrap();
    info!("Listening on {}", addr);

    axum::serve(listener, app)
        .await
        .unwrap();
}

async fn create_contact(
    State(state): State<AppState>,
    Json(payload): Json<ContactCreate>,
) -> Result<(StatusCode, Json<Contact>), (StatusCode, String)> {
    use schema::contacts::dsl::contacts;

    let mut conn = state.pool.get().await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let contact = insert_into(contacts)
        .values(NewContact {
            id: Uuid::new_v4(),
            external_id: payload.external_id,
            phone_number: payload.phone_number,
            date_created: Utc::now(),
            date_updated: Utc::now(),
        })
        .get_result::<Contact>(&mut conn)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok((StatusCode::CREATED, Json(contact)))
}

async fn list_contacts(
    State(state): State<AppState>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (StatusCode, String)> {
    use schema::contacts::dsl::{contacts, external_id, phone_number};

    let mut conn = state.pool.get().await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    let mut query = contacts.into_boxed();

    if let Some(ext_id) = params.external_id {
        query = query.filter(external_id.eq(ext_id));
    }

    if let Some(phone) = params.phone_number {
        query = query.filter(phone_number.like(format!("%{phone}%")));
    }

    let result = query
        .limit(params.limit.unwrap_or(100))
        .offset(params.offset.unwrap_or(0))
        .load::<Contact>(&mut conn)
        .await
        .map_err(|e| (StatusCode::INTERNAL_SERVER_ERROR, e.to_string()))?;

    Ok(Json(result))
}
