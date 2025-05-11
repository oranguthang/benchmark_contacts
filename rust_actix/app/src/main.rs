use actix_web::{web, App, HttpServer, HttpResponse, Responder};
use serde::{Deserialize, Serialize};
use diesel::prelude::*;
use diesel::r2d2::{self, ConnectionManager};
use uuid::Uuid;
use std::env;
use dotenv::dotenv;

#[derive(Queryable, Serialize)]
struct Contact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
    date_created: String,
    date_updated: String,
}

#[derive(Deserialize, Insertable)]
#[table_name = "contacts"]
struct NewContact {
    id: Uuid,
    external_id: i32,
    phone_number: String,
}

#[derive(Deserialize)]
struct QueryParams {
    external_id: Option<i32>,
    phone_number: Option<String>,
    limit: Option<i32>,
    offset: Option<i32>,
}

type DbPool = r2d2::Pool<ConnectionManager<PgConnection>>;

#[actix_web::post("/contacts")]
async fn create_contact(
    contact: web::Json<ContactCreate>,
    pool: web::Data<DbPool>,
) -> impl Responder {
    use crate::schema::contacts;

    let conn = pool.get().expect("couldn't get db connection from pool");

    let new_contact = NewContact {
        id: Uuid::new_v4(),
        external_id: contact.external_id,
        phone_number: contact.phone_number.clone(),
    };

    let inserted_contact: Contact = diesel::insert_into(contacts::table)
        .values(&new_contact)
        .get_result(&conn)
        .expect("Error saving new contact");

    HttpResponse::Created().json(inserted_contact)
}

#[actix_web::get("/contacts")]
async fn get_contacts(
    params: web::Query<QueryParams>,
    pool: web::Data<DbPool>,
) -> impl Responder {
    use crate::schema::contacts::dsl::*;

    let conn = pool.get().expect("couldn't get db connection from pool");

    let limit_value = params.limit.unwrap_or(10000);
    let offset_value = params.offset.unwrap_or(0);

    let mut query = contacts.into_boxed();

    if let Some(ext_id) = params.external_id {
        query = query.filter(external_id.eq(ext_id));
    }

    if let Some(ref phone) = params.phone_number {
        query = query.filter(phone_number.like(format!("%{}%", phone)));
    }

    let results = query
        .offset(offset_value.into())
        .limit(limit_value.into())
        .load::<Contact>(&conn)
        .expect("Error loading contacts");

    HttpResponse::Ok().json(results)
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    dotenv().ok();

    let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let connection_pool = establish_connection_pool(&database_url);

    HttpServer::new(move || {
        App::new()
            .app_data(web::Data::new(connection_pool.clone()))
            .service(create_contact)
            .service(get_contacts)
    })
    .bind("0.0.0.0:8080")?
    .run()
    .await
}

fn establish_connection_pool(database_url: &str) -> DbPool {
    let manager = ConnectionManager::<PgConnection>::new(database_url);
    r2d2::Pool::builder()
        .build(manager)
        .expect("Failed to create pool.")
}
