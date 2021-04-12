package main

import (
	"encoding/json"
	"fmt"
	"time"
)

func update_temp_user_cache(b []byte) bool {
	var m Temp_partial_user_info
	err := json.Unmarshal(b, &m)
	fmt.Println(m)
	if err != nil {
		return false
	}

	if m.Email == "" || m.Rand == 0 {
		fmt.Println("NULL args!")
		return false
	}

	transactions_append(m)
	return true
}

func get_new_api_key() {

}

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
}
