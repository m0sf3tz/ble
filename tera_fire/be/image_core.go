package main

import (
	"bytes"
	"fmt"
	"image"
	_ "image/jpeg"
	"image/png"
	"log"
	"math/rand"
	_ "os"
	_ "time"

	_ "github.com/nfnt/resize"
)

const VERTICAL_PIXELS = 8
const HORIZONTAL_PIXELS = 8
const BYTES_PER_PIXEL = 4 // RBG + ALPHA

func get_image() []byte {
	pixels := make([]byte, VERTICAL_PIXELS*HORIZONTAL_PIXELS*4) // slice of your gray pixels, size of 100x100

	for i := 0; i < VERTICAL_PIXELS*HORIZONTAL_PIXELS*4; i = i + BYTES_PER_PIXEL {

		x := rand.Intn(2)
		fmt.Println(x)
		if x == 0 {
			pixels[i] = byte(255)
			pixels[i+1] = byte(255)
			pixels[i+2] = byte(255)
		} else {
			pixels[i] = byte(0)
			pixels[i+1] = byte(0)
			pixels[i+2] = byte(0)
		}
		pixels[i+3] = byte(255)
	}

	img := image.NewRGBA(image.Rect(0, 0, VERTICAL_PIXELS, HORIZONTAL_PIXELS))
	img.Pix = pixels

	//scaled := resize.Resize(400, 400, img, resize.Lanczos3)
	/*
		f, err := os.Create("img.jpg")
		if err != nil {
			panic(err)
		}
		defer f.Close()
		jpeg.Encode(f, scaled, nil)
	*/

	buf := make([]byte, 0, 1)
	w := bytes.NewBuffer(buf)

	if err := png.Encode(w, img); err != nil {
		log.Fatal(err)
	}

	return w.Bytes()
}

func is_there_fire(api_key string) bool {
	return true
}
