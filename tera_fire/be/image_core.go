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

func get_image(api_key string) []byte {
	pixels, _, valid := db_get_sensor_reading(api_key)
	if !valid {
		return nil
	}

	rgba_matrix := make([]byte, VERTICAL_PIXELS*HORIZONTAL_PIXELS*4)

	for i := 0; i < VERTICAL_PIXELS*HORIZONTAL_PIXELS*4; i = i + BYTES_PER_PIXEL {
		rgba_matrix[i] = 255   // pixels[i/BYTES_PER_PIXEL]
		rgba_matrix[i+1] = 144 //pixels[i/BYTES_PER_PIXEL]
		rgba_matrix[i+2] = pixels[i/BYTES_PER_PIXEL]
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
