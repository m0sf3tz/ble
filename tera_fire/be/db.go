package main

import (
	"database/sql"
	"encoding/json"
	"fmt"
	_ "github.com/lib/pq"
	"log"
	"math/rand"
	"time"
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

var letters = []byte("123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")

type Thermal_meta_data struct {
	Image_age   int
	Image_valid bool
}

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

func api_key_in_db(api_key string) bool {
	row := db.QueryRow(`SELECT COUNT(*) FROM users WHERE api_key = $1`, api_key)
	var count int
	err := row.Scan(&count)
	if err != nil {
		fmt.Println(err)
	}
	if count == 0 {
		// API is NOT in database
		return false
	}
	return true
}

func generate_api_key() string {
	loop_count := 0
	api_key := randSeq(API_KEY_LEN)

	for {
		api_in_db := api_key_in_db(api_key)
		if !api_in_db {
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

func db_update_post(api_key string, sensor_reading []byte) {
	_, err := db.Exec(`update users set post_stamp = $1, sensor_reading = $2 where api_key = $3;`, time.Now().Unix(), sensor_reading, api_key)
	if err != nil {
		log.Fatal(err)
	}
}

// returns meta data about latest image (so far, only how old the thermal capture is)
func db_get_latest_capture_meta_data(api_key string) (bool, []byte) {

	// wasetul, fetching more than we need: TODO: fix
	_, timestamp, valid := db_get_sensor_reading(api_key)

	var meta Thermal_meta_data

	// Calculate how old the stored image is
	image_age := int(time.Now().Unix()) - timestamp

	if valid {
		meta.Image_valid = valid
		meta.Image_age = image_age
	} else {
		meta.Image_valid = false
	}

	bytes, err := json.Marshal(meta)
	if err != nil {
		fmt.Println("Failed to fetch...")
		return false, nil
	}
	fmt.Println(bytes)

	return true, bytes
}

func db_get_sensor_reading(api_key string) ([]byte, int, bool) {
	var post_stamp int
	var bytea []byte

	row := db.QueryRow(`select sensor_reading, post_stamp from users where api_key = $1;`, api_key)
	switch err := row.Scan(&bytea, &post_stamp); err {
	case sql.ErrNoRows:
		fmt.Println("No such thermel image exists!")
		return nil, 0, false
	case nil:
		return bytea, post_stamp, true
	default:
		fmt.Println("Could not fetch details", err)
		return nil, 0, false
	}
}

func db_init() {
	// init the random seed used to generate API keys
	rand.Seed(time.Now().UnixNano())

	db_connect()
}
