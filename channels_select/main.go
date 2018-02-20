package main

import "fmt"
import "time"

func f(ch <-chan string) {
	for {
		select { // HL
		case str := <-ch: // HL
			fmt.Println(str) // HL
		case <-time.After(time.Second): // HL
			fmt.Println("deadline") // HL
		}
	}
}

func main() {
	ch := make(chan string)
	go f(ch)
	time.Sleep(3500 * time.Millisecond)
	ch <- "*** some data"
	time.Sleep(3500 * time.Millisecond)
}
