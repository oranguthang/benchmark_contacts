package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"runtime"
	"strconv"
	"time"

	"github.com/gofiber/fiber/v2"
	"github.com/jackc/pgx/v5/pgxpool"
)

type Contact struct {
	ID          string    `json:"id"`
	ExternalID  int       `json:"external_id"`
	PhoneNumber string    `json:"phone_number"`
	DateCreated time.Time `json:"date_created"`
	DateUpdated time.Time `json:"date_updated"`
}

var db *pgxpool.Pool

func main() {
    numCPU, _ := strconv.Atoi(os.Getenv("CPU_CORES"))
    if numCPU <= 0 {
        numCPU = 8
    }
	runtime.GOMAXPROCS(numCPU)

	var err error
	dsn := os.Getenv("DATABASE_URL")
	if dsn == "" {
		dsn = "postgres://user:password@db:5432/contacts_db"
	}

	config, err := pgxpool.ParseConfig(dsn)
	if err != nil {
		log.Fatalf("Unable to parse database config: %v\n", err)
	}

	poolSize := numCPU * 4
	config.MaxConns = int32(poolSize)
	config.MinConns = int32(poolSize / 2)

	db, err = pgxpool.NewWithConfig(context.Background(), config)
	if err != nil {
		log.Fatalf("Unable to connect to database: %v\n", err)
	}
	defer db.Close()

	app := fiber.New()

	app.Post("/contacts", createContact)
	app.Get("/contacts", getContacts)
	app.Get("/ping", ping)

	fmt.Printf("Server is running on :8080 using %d CPU cores and database pool size %d", numCPU, poolSize)
	log.Fatal(app.Listen(":8080"))
}

func createContact(c *fiber.Ctx) error {
	type ContactInput struct {
		ExternalID  int    `json:"external_id"`
		PhoneNumber string `json:"phone_number"`
	}

	var input ContactInput
	if err := c.BodyParser(&input); err != nil {
		return fiber.NewError(fiber.StatusBadRequest, err.Error())
	}

	var id string
	query := "INSERT INTO contacts (external_id, phone_number) VALUES ($1, $2) RETURNING id"
	err := db.QueryRow(context.Background(), query, input.ExternalID, input.PhoneNumber).Scan(&id)
	if err != nil {
		return fiber.NewError(fiber.StatusInternalServerError, err.Error())
	}

	contact := Contact{
		ID:          id,
		ExternalID:  input.ExternalID,
		PhoneNumber: input.PhoneNumber,
		DateCreated: time.Now().UTC(),
		DateUpdated: time.Now().UTC(),
	}

	return c.Status(fiber.StatusCreated).JSON(contact)
}

func getContacts(c *fiber.Ctx) error {
	externalID := c.Query("external_id")
	phoneNumber := c.Query("phone_number")
	limit, _ := strconv.Atoi(c.Query("limit", "10000"))
	offset, _ := strconv.Atoi(c.Query("offset", "0"))

	if limit > 10000 {
		limit = 10000
	}

	query := "SELECT id, external_id, phone_number, date_created, date_updated FROM contacts WHERE TRUE"
	var args []interface{}
	argIdx := 1

	if externalID != "" {
		query += fmt.Sprintf(" AND external_id = $%d", argIdx)
		args = append(args, externalID)
		argIdx++
	}

	if phoneNumber != "" {
		query += fmt.Sprintf(" AND phone_number = $%d", argIdx)
		args = append(args, phoneNumber)
		argIdx++
	}

	query += fmt.Sprintf(" LIMIT %d OFFSET %d", limit, offset)

	rows, err := db.Query(context.Background(), query, args...)
	if err != nil {
		return fiber.NewError(fiber.StatusInternalServerError, err.Error())
	}
	defer rows.Close()

	var contacts []Contact
	for rows.Next() {
		var ct Contact
		err := rows.Scan(&ct.ID, &ct.ExternalID, &ct.PhoneNumber, &ct.DateCreated, &ct.DateUpdated)
		if err != nil {
			return fiber.NewError(fiber.StatusInternalServerError, err.Error())
		}
		contacts = append(contacts, ct)
	}

	return c.JSON(contacts)
}

func ping(c *fiber.Ctx) error {
	return c.SendString("pong")
}
