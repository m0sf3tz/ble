package main

import (
	"bytes"
	"image"
	_ "image/jpeg"
	"image/png"
	"log"
)

func get_image() []byte {
	pixels := make([]byte, 256*256*4) // slice of your gray pixels, size of 100x100
	for i := 0; i < 256*256; i = i + 4 {
		pixels[i] = byte(0)
		pixels[i+1] = byte(27)
		pixels[i+2] = byte(70)
		pixels[i+3] = byte(255)
	}

	img := image.NewRGBA(image.Rect(0, 0, 100, 100))
	img.Pix = pixels

	buf := make([]byte, 0, 1)
	w := bytes.NewBuffer(buf)

	if err := png.Encode(w, img); err != nil {
		log.Fatal(err)
	}

	return w.Bytes()
}
