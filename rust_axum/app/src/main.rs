use axum::{
    extract::{Query, State},
    routing::{post},
    Json, Router,
};
use diesel::prelude::*;
use diesel::r2d2::{self, ConnectionManager};
use dotenv::dotenv;
use serde::{Deserialize, Serialize};
use std::{env, net::SocketAddr, sync::Arc};
use thiserror::Error;
use tower_http::trace::TraceLayer;
use uuid::Uuid;
use hyper::Server;

mod schema;
use crate::schema::contacts;

type DbPool = r2d2::Pool<ConnectionManager<PgConnection>>;

#[derive(Debug, Serialize, Queryable)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: String,
    date_updated: String,
}

#[derive(Debug, Deserialize, Insertable)]
#[diesel(table_name = contacts)]
struct NewContact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
}

#[derive(Debug, Deserialize)]
struct CreateContactRequest {
    external_id: i32,
    phone_number: String,
}

#[derive(Debug, Deserialize)]
struct ContactQuery {
    external_id: Option<i32>,
    phone_number: Option<String>,
    limit: Option<i64>,
    offset: Option<i64>,
}

#[derive(Clone)]
struct AppState {
    db_pool: Arc<DbPool>,
}

#[derive(Debug, Error)]
enum ApiError {
    #[error("Database error")]
    DatabaseError(#[from] diesel::result::Error),

    #[error("Connection pool error")]
    PoolError(#[from] r2d2::Error),
}

impl Into<axum::http::StatusCode> for ApiError {
    fn into(self) -> axum::http::StatusCode {
        match self {
            ApiError::DatabaseError(_) => axum::http::StatusCode::INTERNAL_SERVER_ERROR,
            ApiError::PoolError(_) => axum::http::StatusCode::SERVICE_UNAVAILABLE,
        }
    }
}

#[tokio::main]
async fn main() {
    dotenv().ok();

    let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let pool = Arc::new(establish_connection_pool(&database_url));

    let app_state = AppState { db_pool: pool };

    let app = Router::new()
        .route("/contacts", post(create_contact).get(get_contacts))
        .with_state(app_state)
        .layer(TraceLayer::new_for_http());

    let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
    println!("Server running at http://{}", addr);

    Server::bind(&addr)
        .serve(app.into_make_service())
        .await
        .unwrap();
}

async fn create_contact(
    State(state): State<AppState>,
    Json(payload): Json<CreateContactRequest>,
) -> Result<Json<Contact>, (axum::http::StatusCode, String)> {
    use crate::schema::contacts::dsl::*;

    let conn = state.db_pool.get().map_err(|e| {
        (
            axum::http::StatusCode::SERVICE_UNAVAILABLE,
            format!("DB pool error: {}", e),
        )
    })?;

    let new_contact = NewContact {
        id: Uuid::new_v4(),
        external_id: payload.external_id,
        phone_number: payload.phone_number,
    };

    let inserted: Contact = diesel::insert_into(contacts)
        .values(&new_contact)
        .get_result(&conn)
        .map_err(|e| {
            (
                axum::http::StatusCode::INTERNAL_SERVER_ERROR,
                format!("DB insert error: {}", e),
            )
        })?;

    Ok(Json(inserted))
}

async fn get_contacts(
    State(state): State<AppState>,
    Query(params): Query<ContactQuery>,
) -> Result<Json<Vec<Contact>>, (axum::http::StatusCode, String)> {
    use crate::schema::contacts::dsl::*;

    let conn = state.db_pool.get().map_err(|e| {
        (
            axum::http::StatusCode::SERVICE_UNAVAILABLE,
            format!("DB pool error: {}", e),
        )
    })?;

    let mut query = contacts.into_boxed();

    if let Some(ext_id) = params.external_id {
        query = query.filter(external_id.eq(ext_id));
    }

    if let Some(phone) = &params.phone_number {
        query = query.filter(phone_number.like(format!("%{}%", phone)));
    }

    let results = query
        .limit(params.limit.unwrap_or(10000))
        .offset(params.offset.unwrap_or(0))
        .load::<Contact>(&conn)
        .map_err(|e| {
            (
                axum::http::StatusCode::INTERNAL_SERVER_ERROR,
                format!("DB select error: {}", e),
            )
        })?;

    Ok(Json(results))
}

fn establish_connection_pool(database_url: &str) -> DbPool {
    let manager = ConnectionManager::<PgConnection>::new(database_url);
    r2d2::Pool::builder()
        .build(manager)
        .expect("Failed to create pool.")
}
