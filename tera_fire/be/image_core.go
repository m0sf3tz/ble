package main

import (
	"bytes"
	"fmt"
	"image"
	"image/png"
	"log"

	"github.com/nfnt/resize"
)

const VERTICAL_PIXELS = 8
const HORIZONTAL_PIXELS = 8
const BYTES_PER_PIXEL = 4 // RBG + ALPHA
const PIXEL_GAIN = 10

func get_image(api_key string) []byte {
	pixels, _, valid := db_get_sensor_reading(api_key)
	if !valid {
		return nil
	}

	rgba_matrix := make([]byte, VERTICAL_PIXELS*HORIZONTAL_PIXELS*4)

	// high gain (10), 20 is degrees, assuming room temperature"
	for i := 0; i < VERTICAL_PIXELS*HORIZONTAL_PIXELS*4; i = i + BYTES_PER_PIXEL {

		r := (int(pixels[i/BYTES_PER_PIXEL]) - 20) * (255 / 100) * 10
		if r > 255 {
			r = 255
		} else if 0 > r {
			r = 0
		}

		b := 255 - (int(pixels[i/BYTES_PER_PIXEL])-20)*(255/100)*10
		if b > 255 {
			b = 255
		} else if 0 > b {
			b = 0
		}

		rgba_matrix[i] = byte(r)
		rgba_matrix[i+1] = 0
		rgba_matrix[i+2] = byte(b)
		rgba_matrix[i+3] = 255 //alpha
	}

	img := image.NewRGBA(image.Rect(0, 0, VERTICAL_PIXELS, HORIZONTAL_PIXELS))
	img.Pix = rgba_matrix
	scaled := resize.Resize(400, 400, img, resize.Lanczos3)

	buf := make([]byte, 0, 1)
	w := bytes.NewBuffer(buf)

	if err := png.Encode(w, scaled); err != nil {
		log.Fatal(err)
	}

	fmt.Println("bytes :=", w.Bytes())
	return w.Bytes()
}

func is_there_fire(sensor_data []byte) bool {
	return true
}
