package main

import (
	"fmt"
	"net/http"
)

func main() {
	fmt.Println("Starting HTTP server on :8080...")

	http.ListenAndServe(":8080", http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprint(w, "hello")
	}))
}
