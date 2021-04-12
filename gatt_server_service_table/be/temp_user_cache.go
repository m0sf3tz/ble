package main

import (
	"fmt"
	"sync"
	"time"
)

var mu sync.Mutex
var pending_signups_map map[uint64]Temp_partial_user_info

type Temp_partial_user_info struct {
	Email     string
	Lat       string
	Long      string
	Rand      uint64
	Timestamp int64
}

func transaction_scan_timeout() {
	mu.Lock()
	for k := range pending_signups_map {
		if time_ms_since_timestamp(pending_signups_map[k].Timestamp) > 10000 {
			fmt.Println("timeout on random_uuid = : ", k)

			// pop from map
			delete(pending_signups_map, k)
		}
	}
	mu.Unlock()
}

func transactions_append(m Temp_partial_user_info) {
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

func get_user_from_cache(rand uint64) (bool, Temp_partial_user_info) {
	var user Temp_partial_user_info
	mu.Lock()
	user, ok := pending_signups_map[rand]
	mu.Unlock()
	if ok {
		return true, user
	} else {
		return false, Temp_partial_user_info{}
	}
}

func scan_timeout_routine() {
	time.Sleep(time.Second * 15)
	transaction_scan_timeout()
}

func init_user_cache() {
	pending_signups_map = make(map[uint64]Temp_partial_user_info)
	go scan_timeout_routine()
}
