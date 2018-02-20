package main

import (
	"fmt"
	"time"
)

func printGo() {
	for i := 1; ; i++ {
		fmt.Printf("%d go\n", i)
		time.Sleep(1 * time.Second)
	}
}

func main() {
	go printGo() // HL
	time.Sleep(10 * time.Second)
}
