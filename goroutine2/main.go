package main

import (
	"fmt"
	"time"
)

func main() {
	go func() {
		for i := 1; ; i++ {
			fmt.Printf("%d go\n", i)
			time.Sleep(1 * time.Second)
		}
	}()

	time.Sleep(10 * time.Second)
}
