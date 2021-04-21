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

		if err != nil {
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		if new_user.Rand == "" {
			fmt.Println("Error, rand is null!")
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		if !update_temp_user_cache(new_user) {
			w.WriteHeader(http.StatusBadRequest)
			return
		}
	}
}

func provision_finalize(w http.ResponseWriter, req *http.Request) {
	if req.Method == "GET" {
		fmt.Println("GET!")

		err := req.ParseForm()
		if err != nil {
			panic(err)
		}

		// Make sure this get response is NOT cached...
		w.Header().Add("Cache-Control", "no-cache")
		w.Header().Add("Cache-Control", "max-age=0")

		random_token := req.FormValue("Rand")
		if random_token == "" {
			fmt.Println("Rand is NULL, returning")
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		// Get the User Random token that was provided in provision_start

		ok, user := get_user_from_cache(random_token)
		if ok {
			api_key := generate_api_key()
			if !db_put(user.Email, user.Lat, user.Long, api_key) {
				w.WriteHeader(http.StatusBadRequest)
				fmt.Println("Failed to get user from cache!")
				return
			}
			fmt.Println("Added user!")

			// respond
			response, ok := create_get_request_response(api_key)
			if ok {
				fmt.Println("Responding... new API key = ", string(response))
				w.Write(response)
				return
			} else {
				fmt.Println("Failed to create new reponse request!")
				w.WriteHeader(http.StatusBadRequest)
				return
			}

		} else {
			fmt.Println("No such user!")
			w.WriteHeader(http.StatusBadRequest)
		}
	}
}

func validate_api_key(w http.ResponseWriter, req *http.Request) {
	if req.Method == "GET" {
		fmt.Println("GET!")

		err := req.ParseForm()
		if err != nil {
			panic(err)
		}

		// Make sure this get response is NOT cached...
		w.Header().Add("Cache-Control", "no-cache")
		w.Header().Add("Cache-Control", "max-age=0")

		api_key := req.FormValue("api_key")
		if api_key == "" {
			fmt.Println("api key is NULL, returning")
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		// respond
		response, ok := create_validate_api_response(api_key_in_db(api_key))
		if ok {
			fmt.Println("Responding... API key found!")
			w.Write(response)
			return
		} else {
			fmt.Println("Failed to create new reponse request to validaet api key")
			w.WriteHeader(http.StatusBadRequest)
			return
		}
	}
}

func get_thermal_image(w http.ResponseWriter, req *http.Request) {
	if req.Method == "GET" {
		fmt.Println("GET - check for thermel image")

		err := req.ParseForm()
		if err != nil {
			panic(err)
		}

		// Make sure this get response is NOT cached...
		w.Header().Add("Cache-Control", "no-cache")
		w.Header().Add("Cache-Control", "max-age=0")
		w.Header().Add("Media-Type", "image/png")

		api_key := req.FormValue("api_key")
		if api_key == "" {
			fmt.Println("api key is NULL, returning")
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		w.Write(get_image(api_key))
	}
}

func get_thermal_meta(w http.ResponseWriter, req *http.Request) {
	if req.Method == "GET" {
		fmt.Println("GET!")

		err := req.ParseForm()
		if err != nil {
			panic(err)
		}

		// Make sure this get response is NOT cached...
		w.Header().Add("Cache-Control", "no-cache")
		w.Header().Add("Cache-Control", "max-age=0")
		w.Header().Add("Media-Type", "image/png")

		api_key := req.FormValue("api_key")
		if api_key == "" {
			fmt.Println("api key is NULL, returning")
			w.WriteHeader(http.StatusBadRequest)
			return
		}

		ok, json := db_get_latest_capture_meta_data(api_key)
		if ok {
			w.Write(json)
		}
	}
}

func sensor_post(w http.ResponseWriter, req *http.Request) {
	if req.Method == "POST" {
		fmt.Println("POST!")
		i := req.Body
		b := make([]byte, HTTP_POST_PACKET_SIZE)
		i.Read(b)
		post_sensor_handler(b)
		i.Close()
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
	http.HandleFunc("/api/validate_api_key/", validate_api_key)
	http.HandleFunc("/api/sensor_post/", sensor_post)
	http.HandleFunc("/api/get_thermal_image/", get_thermal_image)
	http.HandleFunc("/api/get_thermal_meta/", get_thermal_meta)
	log.Fatal(s.ListenAndServe())
}
