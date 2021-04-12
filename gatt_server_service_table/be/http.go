package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
)

func final(w http.ResponseWriter, req *http.Request) {
	if req.Method == "POST" {
		fmt.Println("POST!")

		var p Temp_partial_user_info
		err := json.NewDecoder(req.Body).Decode(&p)
		if err != nil {
			http.Error(w, err.Error(), http.StatusBadRequest)
			return
		}

		fmt.Println(p)

	} else if req.Method == "GET" {
	} else {
		fmt.Println("something else!")
	}

}

func main() {
	init_user_cache()
	db_init()

	mux := http.NewServeMux()

	finalHandler := http.HandlerFunc(final)
	mux.Handle("/", finalHandler)

	log.Println("Listening on :3000...")
	err := http.ListenAndServe("192.168.0.189:3000", mux)
	log.Fatal(err)
}
