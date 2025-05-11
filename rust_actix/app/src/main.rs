use actix_web::{web, App, HttpServer, HttpResponse, Responder};
use serde::{Deserialize, Serialize};
use diesel::prelude::*;
use uuid::Uuid;
use std::env;
use actix_rt::System;
use dotenv::dotenv;

#[derive(Deserialize, Serialize)]
struct ContactCreate {
    external_id: i32,
    phone_number: String,
}

#[derive(Queryable, Serialize)]
struct Contact {
    id: i32,
    external_id: i32,
    phone_number: String,
    date_created: String,
    date_updated: String,
}

#[derive(Deserialize)]
struct QueryParams {
    external_id: Option<i32>,
    phone_number: Option<String>,
    limit: Option<i32>,
    offset: Option<i32>,
}

#[actix_web::post("/contacts")]
async fn create_contact(
    contact: web::Json<ContactCreate>,
    pool: web::Data<diesel::PgConnection>,
) -> impl Responder {
    use crate::schema::contacts;

    let new_contact = ContactCreate {
        external_id: contact.external_id,
        phone_number: contact.phone_number.clone(),
    };

    diesel::insert_into(contacts::table)
        .values(&new_contact)
        .get_result::<Contact>(&*pool)
        .expect("Error saving new contact");

    HttpResponse::Created().json(new_contact)
}

#[actix_web::get("/contacts")]
async fn get_contacts(
    params: web::Query<QueryParams>,
    pool: web::Data<diesel::PgConnection>,
) -> impl Responder {
    use crate::schema::contacts::dsl::*;

    let limit = params.limit.unwrap_or(10000);
    let offset = params.offset.unwrap_or(0);

    let mut query = contacts.into_boxed();

    if let Some(external_id) = params.external_id {
        query = query.filter(external_id.eq(external_id));
    }

    if let Some(ref phone_number) = params.phone_number {
        query = query.filter(phone_number.like(format!("%{}%", phone_number)));
    }

    let results = query
        .offset(offset)
        .limit(limit)
        .load::<Contact>(&*pool)
        .expect("Error loading contacts");

    HttpResponse::Ok().json(results)
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    dotenv().ok();

    let database_url = env::var("DATABASE_URL").expect("DATABASE_URL must be set");
    let connection_pool = establish_connection(&database_url);

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

fn establish_connection(database_url: &str) -> diesel::PgConnection {
    diesel::PgConnection::establish(database_url)
        .expect(&format!("Error connecting to {}", database_url))
}
