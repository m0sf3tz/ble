package main

import (
	"database/sql"
	"fmt"
	_ "github.com/lib/pq"
	"log"
	"math/rand"
)

const API_KEY_LEN = 64

const (
	hostname     = "localhost"
	host_port    = 5432
	username     = "sam"
	password     = ""
	databasename = "tera_fire"
)

var db *sql.DB

var letters = []byte("!@#$%^&*()+-*123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")

// to generate API key
// Note: not cryptographically secure!
func randSeq(n int) string {
	var b [64]byte
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b[:])
}

func db_connect() {
	pg_con_string := fmt.Sprintf("host=%s port=%d user=%s  dbname=%s sslmode=disable", hostname, host_port, username, databasename)
	var err error
	db, err = sql.Open("postgres", pg_con_string)

	if err != nil {
		log.Fatal("Could not connect to database!", err)
	}

	// We can also ping our connection which will let us know if our connection is correct /// or not then we put an error-handling code right after that.
	err = db.Ping()
	if err != nil {
		log.Fatal("Could not ping database!", err)
	}
}

func generate_api_key() string {
	loop_count := 0
	api_key := randSeq(API_KEY_LEN)
	for {
		row := db.QueryRow(`SELECT COUNT(*) FROM users WHERE api_key = $1`, api_key)
		var count int
		err := row.Scan(&count)
		if err != nil {
			fmt.Println(err)
		}

		fmt.Println(count)
		if count == 0 {
			break
		}
		// try a new API key
		api_key = randSeq(API_KEY_LEN)
		loop_count++
		if loop_count == 30 {
			log.Fatal("Stuck in a loop...")
		}
	}

	return api_key
}

func db_put(email, lat, long, api_key string) bool {
	if email == "" {
		fmt.Println("email null!")
		return false
	}

	if api_key == "" {
		fmt.Println("API key null!")
		return false
	}

	// if the Email already exists, drop it
	_, err1 := db.Exec("delete from users where user_email=$1", email)
	if err1 != nil {
		log.Fatal(err1)
	}

	_, err2 := db.Exec("INSERT INTO users VALUES ($1, $2, $3, $4);", email, lat, long, api_key)
	if err2 != nil {
		log.Fatal(err2)
	}
	return true
}

func db_init() {
	db_connect()
}
