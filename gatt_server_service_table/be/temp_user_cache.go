package main

import (
	"fmt"
	"sync"
	"time"
)

const TEMP_CACHE_TIMEOUT_IN_MS = 10000

var mu sync.Mutex
var pending_signups_map map[string]Temp_partial_user_info

type Temp_partial_user_info struct {
	Email     string
	Lat       string
	Long      string
	Rand      string
	Timestamp int64
}

func transaction_scan_timeout() {
	mu.Lock()
	for k := range pending_signups_map {
		if time_ms_since_timestamp(pending_signups_map[k].Timestamp) > TEMP_CACHE_TIMEOUT_IN_MS {
			fmt.Println("timeout on random_uuid = : ", k)

			// pop from map
			delete(pending_signups_map, k)
		}
	}
	mu.Unlock()
}

func transactions_append(m Temp_partial_user_info) {
	fmt.Println("Adding new user to temp cache...")
	mu.Lock()
	m.Timestamp = time_ms_since_epoch()
	pending_signups_map[m.Rand] = m
	mu.Unlock()
}

func time_ms_since_epoch() int64 {
	return time.Now().UnixNano() / 1e6
}

func time_ms_since_timestamp(timestamp int64) int64 {
	timenow := time.Now().UnixNano() / 1e6
	ret := timenow - timestamp
	return ret
}

func get_user_from_cache(rand string) (bool, Temp_partial_user_info) {
	var user Temp_partial_user_info
	mu.Lock()
	user, ok := pending_signups_map[rand]
	defer mu.Unlock()

	if ok {
		delete(pending_signups_map, rand)
		return true, user
	} else {
		return false, Temp_partial_user_info{}
	}
}

func scan_timeout_routine() {
	for {
		time.Sleep(time.Second * 15)
		transaction_scan_timeout()
	}
}

func init_user_cache() {
	pending_signups_map = make(map[string]Temp_partial_user_info)
	go scan_timeout_routine()
}
