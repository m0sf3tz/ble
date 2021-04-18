package main

import (
	"fmt"
)

const SENSOR_VAL_LEN = 64
const HTTP_POST_PACKET_SIZE = SENSOR_VAL_LEN + API_KEY_LEN

func post_sensor_handler(device_post_body []byte) bool {
	if len(device_post_body) != HTTP_POST_PACKET_SIZE {
		fmt.Println("Incorrect packet size!")
		return false
	}

	api := device_post_body[:API_KEY_LEN]
	if !api_key_in_db(string(api)) {
		fmt.Println("API key not in database!")
		return false
	}

	sensor_reading := device_post_body[API_KEY_LEN:]
	fmt.Println("Reading: ", sensor_reading)

	db_update_post(string(api), sensor_reading)
	fmt.Println("API_KEY := ", api)
	return true
}
