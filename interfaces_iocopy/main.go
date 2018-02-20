package main

import (
	"io"
	"os"
	"strings"
)

func main() {
	// strings.NewReader return and io.Reader from a string
	r := strings.NewReader("some io.Reader stream to be read\n")

	// os.Stdout is an io.Writer
	io.Copy(os.Stdout, r)
}
