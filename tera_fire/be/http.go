package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"
)

func provision_request(w http.ResponseWriter, req *http.Request) {
	if req.Method == "POST" {
		fmt.Println("POST!")

		var new_user Temp_partial_user_info
		err := json.NewDecoder(req.Body).Decode(&new_user)

		fmt.Println(new_user)

		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}
		fmt.Println(update_temp_user_cache(new_user))
	}
}

func provision_finalize(w http.ResponseWriter, req *http.Request) {
	if req.Method == "GET" {
		fmt.Println("GET!")

		err := req.ParseForm()
		if err != nil {
			panic(err)
		}

		random_token := req.FormValue("Rand")
		if random_token == "" {
			fmt.Println("Rand is NULL, returning")
		}

		// Get the User Random token that was provided in provision_start

		ok, user := get_user_from_cache(random_token)
		if ok {
			api_key := generate_api_key()
			if !db_put(user.Email, user.Lat, user.Long, api_key) {
				fmt.Println("Failed to insert new value")
			}
			fmt.Println("Added user!")

			// respond
			response, ok := create_get_request_response(api_key)
			if ok {
				fmt.Println("Responding... new API key = ", string(response))
				w.Write(response)
			}

		} else {
			fmt.Println("No such user!")
		}
	}
}

func main() {
	init_user_cache()
	db_init()

	s := &http.Server{
		Addr:           "192.168.0.189:3000",
		ReadTimeout:    10 * time.Second,
		WriteTimeout:   10 * time.Second,
		MaxHeaderBytes: 1 << 20,
	}

	http.HandleFunc("/api/provision_request/", provision_request)
	http.HandleFunc("/api/provision_finalize/", provision_finalize)
	//err := http.ListenAndServe("192.168.0.189:3000", nil)
	log.Fatal(s.ListenAndServe())
}
