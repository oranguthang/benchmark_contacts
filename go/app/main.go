package main

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"

	"github.com/jackc/pgx/v5/pgxpool"
)

type Contact struct {
	ID          int    `json:"id"`
	ExternalID  int    `json:"external_id"`
	PhoneNumber string `json:"phone_number"`
	DateCreated string `json:"date_created"`
	DateUpdated string `json:"date_updated"`
}

var db *pgxpool.Pool

func main() {
	var err error
	dsn := os.Getenv("DATABASE_URL")
	if dsn == "" {
		dsn = "postgres://postgres:postgres@db:5432/postgres"
	}

	db, err = pgxpool.New(context.Background(), dsn)
	if err != nil {
		log.Fatalf("Unable to connect to database: %v\n", err)
	}
	defer db.Close()

	http.HandleFunc("/contacts", contactsHandler)

	fmt.Println("Server is running on :8080")
	log.Fatal(http.ListenAndServe(":8080", nil))
}

func contactsHandler(w http.ResponseWriter, r *http.Request) {
	switch r.Method {
	case http.MethodPost:
		createContact(w, r)
	case http.MethodGet:
		getContacts(w, r)
	default:
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
	}
}

func createContact(w http.ResponseWriter, r *http.Request) {
	var c Contact
	err := json.NewDecoder(r.Body).Decode(&c)
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	_, err = db.Exec(context.Background(),
		"INSERT INTO contacts (external_id, phone_number) VALUES ($1, $2)",
		c.ExternalID, c.PhoneNumber)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusCreated)
}

func getContacts(w http.ResponseWriter, r *http.Request) {
	params := r.URL.Query()

	externalID := params.Get("external_id")
	phoneNumber := params.Get("phone_number")
	limit := 10000
	offset := 0

	if limitParam := params.Get("limit"); limitParam != "" {
		parsedLimit, err := strconv.Atoi(limitParam)
		if err == nil {
			limit = parsedLimit
		}
	}

	if offsetParam := params.Get("offset"); offsetParam != "" {
		parsedOffset, err := strconv.Atoi(offsetParam)
		if err == nil {
			offset = parsedOffset
		}
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
	}

	query += fmt.Sprintf(" LIMIT %d OFFSET %d", limit, offset)

	rows, err := db.Query(context.Background(), query, args...)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	var contacts []Contact
	for rows.Next() {
		var c Contact
		err := rows.Scan(&c.ID, &c.ExternalID, &c.PhoneNumber, &c.DateCreated, &c.DateUpdated)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		contacts = append(contacts, c)
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(contacts)
}
