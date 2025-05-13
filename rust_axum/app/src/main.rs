use axum::{
    extract::{Query, State},
    routing::{get, post},
    Json, Router,
};
use axum::serve;
use tokio::net::TcpListener;
use chrono::{DateTime, Utc};
use diesel::prelude::*;
use diesel::r2d2::{self, ConnectionManager};
use serde::{Deserialize, Serialize};
use std::{env, net::SocketAddr, sync::Arc};
use dotenv::dotenv;
use uuid::Uuid;
use tracing;
use tracing_subscriber;
use tower_http::trace::TraceLayer;
use axum::http::StatusCode;

mod schema;

type DbPool = r2d2::Pool<ConnectionManager<PgConnection>>;

#[derive(Debug, Serialize, Queryable)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: DateTime<Utc>,
    date_updated: DateTime<Utc>,
}

#[derive(Debug, Deserialize, Insertable)]
#[diesel(table_name = schema::contacts)]
struct NewContact {
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

#[tokio::main]
async fn main() {
    dotenv().ok();
    tracing_subscriber::fmt::init();

    let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let pool = establish_connection_pool(&database_url);

    let app_state = Arc::new(AppState { pool });

    let app = Router::new()
        .route("/ping", get(ping))
        .route("/contacts", post(create_contact).get(get_contacts))
        .with_state(app_state)
        .layer(TraceLayer::new_for_http());

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    let listener = TcpListener::bind(&addr).await.unwrap();
    tracing::info!("Listening on {}", addr);

    serve(listener, app)
        .await
        .unwrap();
}

#[derive(Clone)]
struct AppState {
    pool: DbPool,
}

async fn create_contact(
    State(state): State<Arc<AppState>>,
    Json(payload): Json<ContactCreate>,
) -> Result<(StatusCode, Json<Contact>), (StatusCode, String)> {
    use schema::contacts::dsl::*;

    let mut conn = state.pool.get().map_err(internal_error)?;

    let new_contact = NewContact {
        id: Uuid::new_v4(),
        external_id: payload.external_id,
        phone_number: payload.phone_number,
        date_created: chrono::Utc::now(),
        date_updated: chrono::Utc::now(),
    };

    diesel::insert_into(contacts)
        .values(&new_contact)
        .get_result(&mut conn)
        .map(|contact| (StatusCode::CREATED, Json(contact)))
        .map_err(internal_error)
}

async fn get_contacts(
    State(state): State<Arc<AppState>>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (axum::http::StatusCode, String)> {
    use schema::contacts::dsl::*;

    let mut conn = state.pool.get().map_err(internal_error)?;

    let mut query = contacts.into_boxed();

    if let Some(ext_id) = params.external_id {
        query = query.filter(external_id.eq(ext_id));
    }

    if let Some(ref phone) = params.phone_number {
        query = query.filter(phone_number.like(format!("%{}%", phone)));
    }

    let limit_val = params.limit.unwrap_or(10000);
    let offset_val = params.offset.unwrap_or(0);

    query
        .limit(limit_val)
        .offset(offset_val)
        .load(&mut conn)
        .map(Json)
        .map_err(internal_error)
}

async fn ping() -> &'static str {
    "pong"
}

fn establish_connection_pool(database_url: &str) -> DbPool {
    let manager = ConnectionManager::<PgConnection>::new(database_url);
    r2d2::Pool::builder()
        .build(manager)
        .expect("Failed to create pool.")
}

fn internal_error<E: std::fmt::Display>(err: E) -> (axum::http::StatusCode, String) {
    (axum::http::StatusCode::INTERNAL_SERVER_ERROR, format!("{}", err))
}
