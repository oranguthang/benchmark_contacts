package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net/http"
	"os"
	"sync"
	"time"

	"github.com/google/uuid"
)

type Contact struct {
	ID          string `json:"id"`
	ExternalID  int    `json:"external_id"`
	PhoneNumber string `json:"phone_number"`
	DateCreated string `json:"date_created"`
	DateUpdated string `json:"date_updated"`
}

var (
	baseURL      string
	postCount    int
	getCount     int
	postWorkers  int
	getWorkers   int
	phonePrefix  = "+7999"
	maxExtID     = 1000000
	createdMutex sync.Mutex
	created      []Contact
	client       = &http.Client{}
)

func init() {
	flag.StringVar(&baseURL, "base-url", "http://python_aiohttp:8080", "Base URL of the contact service")
	flag.IntVar(&postCount, "post-count", 1000000, "Total number of POST requests")
	flag.IntVar(&getCount, "get-count", 1000000, "Total number of GET requests")
	flag.IntVar(&postWorkers, "post-workers", 100, "Number of concurrent POST workers")
	flag.IntVar(&getWorkers, "get-workers", 100, "Number of concurrent GET workers")
}

func main() {
	flag.Parse()
	rand.Seed(time.Now().UnixNano())
	fmt.Printf("Benchmark started with config:\nBase URL: %s\nPOST: %d (%d workers)\nGET: %d (%d workers)\n\n",
		baseURL, postCount, postWorkers, getCount, getWorkers)

	// POST phase
	startPost := time.Now()
	runPOSTRequests()
	elapsedPost := time.Since(startPost)
	fmt.Printf("Total POST time: %v\n", elapsedPost)

	// GET phase
	startGet := time.Now()
	runGETRequests()
	elapsedGet := time.Since(startGet)
	fmt.Printf("Total GET time: %v\n", elapsedGet)
}

func runPOSTRequests() {
	wg := sync.WaitGroup{}
	jobs := make(chan int, postCount)

	for i := 0; i < postWorkers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for range jobs {
				createContact()
			}
		}()
	}

	for i := 0; i < postCount; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
}

func createContact() {
	extID := rand.Intn(maxExtID)
	phone := fmt.Sprintf("%s%07d", phonePrefix, rand.Intn(10000000))

	body := map[string]interface{}{
		"external_id":  extID,
		"phone_number": phone,
	}
	jsonBody, _ := json.Marshal(body)

	resp, err := client.Post(baseURL+"/contacts", "application/json", bytes.NewBuffer(jsonBody))
	if err != nil {
		log.Printf("POST error: %v", err)
		return
	}
	defer resp.Body.Close()

	var c Contact
	respBytes, _ := io.ReadAll(resp.Body)
	err = json.Unmarshal(respBytes, &c)
	if err == nil {
		createdMutex.Lock()
		created = append(created, c)
		createdMutex.Unlock()
	}
}

func runGETRequests() {
	if len(created) == 0 {
		fmt.Println("No contacts created, skipping GET requests")
		os.Exit(1)
	}

	wg := sync.WaitGroup{}
	jobs := make(chan int, getCount)

	for i := 0; i < getWorkers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for j := range jobs {
				doGETRequest(j)
			}
		}()
	}

	for i := 0; i < getCount; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
}

func doGETRequest(i int) {
	var url string

	switch {
	case i < getCount/10*3:
		contact := created[rand.Intn(len(created))]
		url = fmt.Sprintf("%s/contacts?phone_number=%s&limit=10000&offset=0", baseURL, contact.PhoneNumber)
	case i < getCount/10*6:
		contact := created[rand.Intn(len(created))]
		url = fmt.Sprintf("%s/contacts?external_id=%d&limit=10000&offset=0", baseURL, contact.ExternalID)
	default:
		randomID := rand.Intn(100001)
		url = fmt.Sprintf("%s/contacts?external_id=%d&limit=10000&offset=0", baseURL, randomID)
	}

	resp, err := client.Get(url)
	if err != nil {
		log.Printf("GET error: %v", err)
		return
	}
	defer resp.Body.Close()
	io.Copy(io.Discard, resp.Body)
}
