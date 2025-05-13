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
	"runtime"
	"strconv"
	"sync"
	"time"

	"github.com/schollz/progressbar/v3"
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
	client       *http.Client
)

func init() {
	flag.StringVar(&baseURL, "base-url", "http://python_aiohttp:8080", "Base URL of the contact service")
	flag.IntVar(&postCount, "post-count", 1000000, "Total number of POST requests")
	flag.IntVar(&getCount, "get-count", 1000000, "Total number of GET requests")
	flag.IntVar(&postWorkers, "post-workers", 100, "Number of concurrent POST workers")
	flag.IntVar(&getWorkers, "get-workers", 100, "Number of concurrent GET workers")
}

func main() {
	numCPU, _ := strconv.Atoi(os.Getenv("CPU_CORES"))
	if numCPU <= 0 {
		numCPU = runtime.NumCPU()
	}
	runtime.GOMAXPROCS(numCPU)

	// Initialize HTTP client with connection pooling
	client = &http.Client{
		Transport: &http.Transport{
			MaxIdleConns:        postWorkers + getWorkers,
			MaxIdleConnsPerHost: postWorkers + getWorkers,
			IdleConnTimeout:     90 * time.Second,
		},
		Timeout: 30 * time.Second,
	}

	flag.Parse()
	fmt.Printf("Benchmark started with config:\nBase URL: %s\nPOST: %d (%d workers)\nGET: %d (%d workers)\nCPU: %d\n\n",
		baseURL, postCount, postWorkers, getCount, getWorkers, numCPU)

	// POST phase
	startPost := time.Now()
	runPOSTRequests()
	elapsedPost := time.Since(startPost)
	fmt.Printf("\nTotal POST time: %v\n", elapsedPost)
	fmt.Printf("POST RPS: %.2f\n", float64(postCount)/elapsedPost.Seconds())

	// GET phase
	startGet := time.Now()
	runGETRequests()
	elapsedGet := time.Since(startGet)
	fmt.Printf("\nTotal GET time: %v\n", elapsedGet)
	fmt.Printf("GET RPS: %.2f\n", float64(getCount)/elapsedGet.Seconds())
}

func runPOSTRequests() {
	wg := sync.WaitGroup{}
	jobs := make(chan int, postWorkers*10) // Buffered channel
	bar := progressbar.NewOptions(postCount,
		progressbar.OptionSetDescription("POST requests"),
		progressbar.OptionSetWidth(30),
		progressbar.OptionSetPredictTime(true),
		progressbar.OptionShowCount(),
		progressbar.OptionSetTheme(progressbar.Theme{
			Saucer:        "#",
			SaucerHead:    ">",
			SaucerPadding: "-",
			BarStart:      "[",
			BarEnd:        "]",
		}),
	)

	for i := 0; i < postWorkers; i++ {
		wg.Add(1)
		seed := time.Now().UnixNano() + int64(i)
		r := rand.New(rand.NewSource(seed))

		go func(r *rand.Rand) {
			defer wg.Done()
			for range jobs {
				createContact(r)
				bar.Add(1)
			}
		}(r)
	}

	for i := 0; i < postCount; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
}

func createContact(r *rand.Rand) {
	extID := r.Intn(maxExtID)
	phone := fmt.Sprintf("%s%07d", phonePrefix, r.Intn(10000000))

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
	if err := json.NewDecoder(resp.Body).Decode(&c); err == nil {
		createdMutex.Lock()
		created = append(created, c)
		createdMutex.Unlock()
	}
}

func runGETRequests() {
	if len(created) == 0 {
		fmt.Println("No contacts created, skipping GET requests")
		return
	}

	wg := sync.WaitGroup{}
	jobs := make(chan int, getWorkers*10)
	bar := progressbar.NewOptions(getCount,
		progressbar.OptionSetDescription("GET requests"),
		progressbar.OptionSetWidth(30),
		progressbar.OptionSetPredictTime(true),
		progressbar.OptionShowCount(),
		progressbar.OptionSetTheme(progressbar.Theme{
			Saucer:        "#",
			SaucerHead:    ">",
			SaucerPadding: "-",
			BarStart:      "[",
			BarEnd:        "]",
		}),
	)

	for i := 0; i < getWorkers; i++ {
		wg.Add(1)
		seed := time.Now().UnixNano() + int64(i)
		r := rand.New(rand.NewSource(seed))

		go func(r *rand.Rand) {
			defer wg.Done()
			for range jobs {
				doGETRequest(r)
				bar.Add(1)
			}
		}(r)
	}

	for i := 0; i < getCount; i++ {
		jobs <- i
	}
	close(jobs)
	wg.Wait()
}

func doGETRequest(r *rand.Rand) {
	var url string

	switch r.Intn(10) {
	case 0, 1, 2: // 30% by phone
		contact := created[r.Intn(len(created))]
		url = fmt.Sprintf("%s/contacts?phone_number=%s&limit=10000&offset=0", baseURL, contact.PhoneNumber)
	case 3, 4, 5, 6, 7, 8: // 60% by external_id
		contact := created[r.Intn(len(created))]
		url = fmt.Sprintf("%s/contacts?external_id=%d&limit=10000&offset=0", baseURL, contact.ExternalID)
	default: // 10% random
		randomID := r.Intn(100001)
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
