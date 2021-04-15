package main

import (
	"encoding/json"
	"fmt"
	_ "time"
)

type User_created_get_response struct {
	Api_key string
}

func update_temp_user_cache(user Temp_partial_user_info) bool {
	if user.Email == "" {
		fmt.Println("NULL args!")
		return false
	}

	transactions_append(user)
	return true
}

func create_get_request_response(api_key string) ([]byte, bool) {
	var response User_created_get_response
	response.Api_key = api_key
	b, err := json.Marshal(response)

	if err != nil {
		fmt.Println("Failed to write...")
		return nil, false
	}
	return b, true
}

/*
func main_test() {
	init_user_cache()
	db_init()

	test := Temp_partial_user_info{"cu2nt", "cunt", "cunt", 34234, 0}
	b, err := json.Marshal(test)
	fmt.Println(err)
	update_temp_user_cache(b)

	test = Temp_partial_user_info{}
	api_key := generate_api_key()
	ok, test := get_user_from_cache(34234)
	fmt.Println(ok, test)

	db_put(test.Email, test.Lat, test.Long, api_key)
	if !db_put("asdasd", test.Lat, test.Long, api_key) {
		fmt.Println("Failed to insert new value")
	}
	time.Sleep(time.Second * 30)
}*/
