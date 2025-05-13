use axum::{
    extract::{Query, State},
    http::StatusCode,
    routing::{get, post},
    Json, Router,
};
use chrono::Utc;
use diesel::{
    prelude::*
};
use diesel_async::{
    pooled_connection::bb8::Pool,
    AsyncPgConnection,
    RunQueryDsl,
};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;
use tokio::net::TcpListener;
use tower_http::trace::TraceLayer;
use tracing::Level;
use uuid::Uuid;
use num_cpus;

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
    external_id: i32,
    phone_number: String,
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
    db_pool: DbPool,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    std::panic::set_hook(Box::new(|panic_info| {
        eprintln!(
            "Custom panic occurred: {}\nLocation: {:?}",
            panic_info,
            panic_info.location()
        );
    }));

    let tokio_runtime = tokio::runtime::Builder::new_multi_thread()
        .worker_threads(num_cpus::get())
        .max_blocking_threads(num_cpus::get() * 2)
        .enable_all()
        .build()?;

    tokio_runtime.block_on(async {
        tracing_subscriber::fmt()
            .with_max_level(Level::INFO)
            .init();

        let db_pool = create_db_pool().await;
        let app_state = AppState { db_pool };

        let app = Router::new()
            .route("/ping", get(ping))
            .route("/contacts",
                post(create_contact)
                .get(list_contacts)
            )
            .with_state(app_state)
            .layer(TraceLayer::new_for_http());

        let addr = SocketAddr::from(([0, 0, 0, 0], 8080));
        let listener = TcpListener::bind(addr).await?;

        tracing::info!(
            "Server started on {} (worker threads: {})",
            addr,
            tokio::runtime::Handle::current().metrics().num_workers()
        );

        axum::serve(listener, app).await?;
        Ok(())
    })
}

async fn ping() -> &'static str {
    "pong"
}

async fn create_contact(
    State(state): State<AppState>,
    Json(payload): Json<ContactCreate>,
) -> Result<(StatusCode, Json<Contact>), (StatusCode, String)> {
    use schema::contacts::dsl::contacts;

    let mut conn = state.db_pool.get().await.map_err(db_error)?;

    let new_contact = NewContact {
        external_id: payload.external_id,
        phone_number: payload.phone_number,
    };

    let contact = diesel::insert_into(contacts)
        .values(&new_contact)
        .get_result::<Contact>(&mut conn)
        .await
        .map_err(db_error)?;

    Ok((StatusCode::CREATED, Json(contact)))
}


async fn list_contacts(
    State(state): State<AppState>,
    Query(params): Query<QueryParams>,
) -> Result<Json<Vec<Contact>>, (StatusCode, String)> {
    let mut conn = state.db_pool.get().await.map_err(db_error)?;

    let mut sql = String::from("SELECT * FROM contacts WHERE 1=1");
    let mut bindings: Vec<(String, String)> = Vec::new();

    if let Some(ext_id) = params.external_id {
        sql.push_str(" AND external_id = $1");
        bindings.push(("external_id".to_string(), ext_id.to_string()));
    }

    if let Some(ref phone) = params.phone_number {
        sql.push_str(" AND phone_number ILIKE $2");
        bindings.push(("phone_number".to_string(), format!("%{}%", phone)));
    }

    let limit_value = params.limit.unwrap_or(100);
    let offset_value = params.offset.unwrap_or(0);

    sql.push_str(&format!(" LIMIT {} OFFSET {}", limit_value, offset_value));

    let mut query = diesel::sql_query(sql);

    for (i, (_, value)) in bindings.iter().enumerate() {
        query = query.bind::<diesel::sql_types::Text, _>(value);
    }

    let results = query
        .load::<Contact>(&mut conn)
        .await
        .map_err(db_error)?;

    Ok(Json(results))
}


async fn create_db_pool() -> DbPool {
    let db_url = std::env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let manager = diesel_async::pooled_connection::AsyncDieselConnectionManager::<AsyncPgConnection>::new(db_url);

    Pool::builder()
        .max_size((num_cpus::get() * 4).try_into().unwrap())
        .build(manager)
        .await
        .expect("Failed to create DB pool")
}

fn db_error<E: std::fmt::Display>(err: E) -> (StatusCode, String) {
    (StatusCode::INTERNAL_SERVER_ERROR, err.to_string())
}
