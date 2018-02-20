package main

import (
	"fmt"
	"time"
)

func f(ch <-chan string) { // HL
	for {
		str := <-ch
		fmt.Println(str)
		time.Sleep(time.Second)
	}
}

func main() {
	ch := make(chan string)
	go f(ch)
	ch <- "Hello"
	ch <- "world"
	ch <- "from"
	ch <- "channel"
	time.Sleep(time.Second)
}
